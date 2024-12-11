/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"

#define NUM_PIXELS 42
#define WS2812_PIN_BASE 0
#define STRIPS 28

// Check the pin is compatible with the platform
#if WS2812_PIN_BASE >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif
// we store value (8 bits of a single color (R/G/B/W) value) for multiple
// strips of pixels, in bit planes. bit plane N has the Nth bit of each strip of pixels.
#define VALUE_PLANE_COUNT (8)
typedef struct
{
    // stored MSB first
    uint32_t planes[VALUE_PLANE_COUNT];
} value_bits_t;

// Just a simple raster display of RGB values for each pixel
uint32_t display[STRIPS][NUM_PIXELS];

static value_bits_t colors[NUM_PIXELS * 3];
// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version

uint current_buffer = 0;
static value_bits_t buffers[2][NUM_PIXELS * 3];

void printBinary(const char *description, unsigned int number)
{
    printf("%s: ", description); // Print the description
    for (int i = 31; i >= 0; i--)
    { // Iterate through the bits
        printf("%c", (number & (1 << i)) ? '1' : '0');
        if (i % 4 == 0 && i != 0)
        { // Add a space every 4 bits
            printf(" ");
        }
    }
    printf("\n"); // Newline at the end
}

/**
 * Put a pixel into the bit plane buffer
 */
static inline void put_pixel(uint strip, uint pixel, uint32_t pixel_rgb)
{

    uint b = pixel_rgb & 0xffu;
    uint g = (pixel_rgb >> 8u) & 0xffu;
    uint r = (pixel_rgb >> 16u) & 0xffu;
    uint v = pixel * 3;

    uint32_t mask = 1 << strip; // The mask for the current strip

    uint color_array[3] = {r, g, b};

    // Iterate through the colors
    for (int i = 0; i < 3; i++)
    { // Each bit plane is 32 bits, one bit for each strip, with the MSB being the first strip
        // There are three bit planes, one for each color
        uint32_t *values = buffers[current_buffer][v + i].planes;
        uint32_t color = color_array[i];
        // Iterate through the 8 bits in each color
        for (uint bit = 0; bit < 8; bit++)
        {
            // Get the current color at this bit plane location
            uint32_t value = values[bit];
            // Calculate the bit we are setting.
            uint color_bit = (color >> (7 - bit)) & 1;
            // Calculate the new value in the bit plane.
            values[bit] = (color_bit) ? (value | (mask)) : (value & ~(mask));
        }
    }
}

// bit plane content dma channel
#define DMA_CHANNEL 0
// chain channel for configuring main dma channel to output from disjoint 8 word fragments of memory
#define DMA_CB_CHANNEL 1

#define DMA_CHANNEL_MASK (1u << DMA_CHANNEL)
#define DMA_CB_CHANNEL_MASK (1u << DMA_CB_CHANNEL)
#define DMA_CHANNELS_MASK (DMA_CHANNEL_MASK | DMA_CB_CHANNEL_MASK)

// start of each value (+1 for NULL terminator)
static uintptr_t fragment_start[NUM_PIXELS * 3 + 1];

// posted when it is safe to output a new set of values
static struct semaphore reset_delay_complete_sem;
// alarm handle for handling delay
alarm_id_t reset_delay_alarm_id;

int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data)
{
    reset_delay_alarm_id = 0;
    sem_release(&reset_delay_complete_sem);
    // no repeat
    return 0;
}

void __isr dma_complete_handler()
{
    if (dma_hw->ints0 & DMA_CHANNEL_MASK)
    {
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;
        // when the dma is complete we start the reset delay timer
        if (reset_delay_alarm_id)
            cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
    }
}

void dma_init(PIO pio, uint sm)
{
    dma_claim_mask(DMA_CHANNELS_MASK);

    // main DMA channel outputs 8 word fragments, and then chains back to the chain channel
    dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_dreq(&channel_config, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL);
    channel_config_set_irq_quiet(&channel_config, true);
    dma_channel_configure(DMA_CHANNEL,
                          &channel_config,
                          &pio->txf[sm],
                          NULL, // set by chain
                          8,    // 8 words for 8 bit planes
                          false);

    // chain channel sends single word pointer to start of fragment each time
    dma_channel_config chain_config = dma_channel_get_default_config(DMA_CB_CHANNEL);
    dma_channel_configure(DMA_CB_CHANNEL,
                          &chain_config,
                          &dma_channel_hw_addr(
                               DMA_CHANNEL)
                               ->al3_read_addr_trig, // ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
                          NULL,                      // set later
                          1,
                          false);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);
}

void output_strips_dma(value_bits_t *bits, uint value_length)
{
    for (uint i = 0; i < value_length; i++)
    {
        fragment_start[i] = (uintptr_t)bits[i].planes; // MSB first
    }
    fragment_start[value_length] = 0;
    dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = (uintptr_t)fragment_start;
}

void print_current_buffer()
{
    printf("Current buffer:\n");

    for (int i = 0; i < NUM_PIXELS * 3; i++)
    {
        printf("Pixel %d: \n", i);
        for (int j = VALUE_PLANE_COUNT - 1; j >= 0; j--)
        {
            for (int k = 0; k < 32; k++)
            {
                printf("%d", (buffers[current_buffer][i].planes[j] >> k) & 1);
            }
            printf("\n");
        }
        printf("\n");
    }
    printf("\n");
}

// DMA the current bit plane to the PIO
// We double buffer so we don't write to the memory while the DMA is reading from it
void show_pixels()
{
    sem_acquire_blocking(&reset_delay_complete_sem);
    output_strips_dma(buffers[current_buffer], NUM_PIXELS * 3);
    // copy current buffer to next buffer
    memcpy(buffers[current_buffer ^ 1], buffers[current_buffer], sizeof(buffers[0]));
    // switch buffers
    current_buffer ^= 1;
}
// Copy pixels from display to bitplanes using put_pixel
void show_display()
{
    for (uint strip = 0; strip < STRIPS; strip++)
    {
        for (uint pixel = 0; pixel < NUM_PIXELS; pixel++)
        {
            put_pixel(strip, pixel, display[strip][pixel]);
        }
    }
    show_pixels();
}
uint64_t timer_start()
{
    return time_us_64();
}

double timer_end(uint64_t start, const char *comment)
{
    uint64_t end_us = time_us_64();
    // Calculate elapsed time in seconds
    double elapsed = (double)(end_us - start);
    printf("%s: %dµs\n", comment, (int)elapsed);
    return elapsed;
}

uint64_t last_sparkle_time = 0;

void run_sparkle()
{
    // Run every 20ms
    if (time_us_64() - last_sparkle_time < 20000)
    {
        return;
    }
    else
    {
        last_sparkle_time = time_us_64();
    }
    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            uint32_t rgb = display[i][j];
            // Randomly every 30th time reset the pixel to white
            if (rand() % 100 == 0)
            {
                rgb = 0xffff00;
            }
            else
            {
                uint r = ((rgb >> 16) & 0xff) * 240 >> 8;
                uint g = ((rgb >> 8) & 0xff) * 240 >> 8;
                uint b = (rgb & 0xff) * 240 >> 8;
                // Fade the pixel
                rgb = (r << 16) | (g << 8) | b;
            }
            display[i][j] = rgb;
        }
    }
}

uint64_t my_timer;
int main()
{
    memset(&display[0], 0, sizeof(display[0]));

    stdio_init_all();
    printf("WS2812 parallel using pin %d\n", WS2812_PIN_BASE);

    PIO pio;
    uint sm;
    uint offset;
    memset(&buffers[0], 0, sizeof(buffers[0]));
    memset(&buffers[1], 0, sizeof(buffers[1]));
    print_current_buffer();
    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_parallel_program, &pio, &sm, &offset, WS2812_PIN_BASE, STRIPS, true);
    hard_assert(success);

    ws2812_parallel_program_init(pio, sm, offset, WS2812_PIN_BASE, STRIPS, 800000);

    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    dma_init(pio, sm);

    while (1)
    {
        // printf("Loop");
        // print_current_buffer();
        show_pixels();

        my_timer = timer_start();
        run_sparkle();
        show_display();
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_parallel_program, pio, sm, offset);
}
