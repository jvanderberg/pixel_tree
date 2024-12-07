/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"

#define FRAC_BITS 0
#define NUM_PIXELS 2
#define WS2812_PIN_BASE 0
#define STRIPS 3

// Check the pin is compatible with the platform
#if WS2812_PIN_BASE >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif





#define VALUE_PLANE_COUNT (8)
// we store value (8 bits + fractional bits of a single color (R/G/B/W) value) for multiple
// strips of pixels, in bit planes. bit plane N has the Nth bit of each strip of pixels.
typedef struct {
    // stored MSB first
    uint32_t planes[VALUE_PLANE_COUNT];
} value_bits_t;

// Add FRAC_BITS planes of e to s and store in d
void add_error(value_bits_t *state, const value_bits_t *colors, const value_bits_t *old_state) {
    // uint32_t carry_plane = 0;
    // // add the FRAC_BITS low planes
    // for (int p = VALUE_PLANE_COUNT - 1; p >= 8; p--) {
    //     uint32_t old_state_plane =old_state->planes[p];
    //     uint32_t colors_plane = colors->planes[p];
    //     state->planes[p] = (old_state_plane ^ colors_plane) ^ carry_plane;
    //     carry_plane = (old_state_plane & colors_plane) | (carry_plane & (colors_plane ^ old_state_plane));
    // }
    // then just ripple carry through the non fractional bits
    for (int p = 7; p >= 0; p--) {
        uint32_t colors_plane = colors->planes[p];
        state->planes[p] = colors_plane;// ^ carry_plane;
        // carry_plane &= s_plane;
    }
}

typedef struct {
    uint8_t *data;
    uint data_len;
    uint frac_brightness; // 256 = *1.0;
} strip_t;

// takes 8 bit color values, multiply by brightness and store in bit planes
void transform_strips(strip_t **strips, uint num_strips, value_bits_t *values, uint value_length,
                       uint frac_brightness) {
    for (uint v = 0; v < value_length; v++) {
        memset(&values[v], 0, sizeof(values[v]));
        for (uint i = 0; i < num_strips; i++) {
            if (v < strips[i]->data_len) {
                // todo clamp?
                uint32_t value = (strips[i]->data[v] * strips[i]->frac_brightness) >> 8u;
                value = (value * frac_brightness) >> 8u;
                for (int j = 0; j < VALUE_PLANE_COUNT && value; j++, value >>= 1u) {
                    if (value & 1u) values[v].planes[VALUE_PLANE_COUNT - 1 - j] |= 1u << i;
                }
            }
        }
    }
}


// requested colors * 4 to allow for RGBW
static value_bits_t colors[NUM_PIXELS * 4];
// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version

uint current_buffer = 0;
static value_bits_t buffers[2][NUM_PIXELS * 4];

void printBinary(const char *description, unsigned int number) {
    printf("%s: ", description); // Print the description
    for (int i = 31; i >= 0; i--) { // Iterate through the bits
        printf("%c", (number & (1 << i)) ? '1' : '0');
        if (i % 4 == 0 && i != 0) { // Add a space every 4 bits
            printf(" ");
        }
    }
    printf("\n"); // Newline at the end
}
static inline void put_pixel(uint strip, uint pixel, uint32_t pixel_rgb) {
    
    uint b = pixel_rgb & 0xffu;
    uint g = (pixel_rgb >> 8u) & 0xffu;
    uint r = (pixel_rgb >> 16u) & 0xffu;
    uint v = pixel *  4;
    printf("put pixel r %d g %d b %d strip %i pixel %i\n", r, g, b, strip, pixel);
    printBinary("G", g);
    printBinary("B", b);
    uint32_t mask = 1 << 31 - strip; // The mask for the current strip
    printBinary("Mask", mask);
    for (int plane=0 ; plane < 3; plane++) {
        uint32_t *values = buffers[current_buffer][v + plane].planes;
        printf("Pos: %i\n", v + plane);
        if (plane == 0) {
            for (uint bit = 0; bit < 7; bit++) {
        
                uint32_t r_value = values[bit];
                printBinary("Before r value", r_value);  
                 uint rbit = (r >> (7 - bit)) & 1;
                printf("R value %d bit %d rbit %d\n", r_value, bit, rbit);
               
                values[bit] = (rbit) ? (r_value | (mask)) : (r_value & ~(mask));
                printBinary("After r value", values[bit]);  
            }
        } else if (plane == 1) {
            for (uint bit = 0; bit < 7; bit++) {
               
                uint32_t g_value = values[bit];
                printf("G value %d bit %d\n", g_value, bit);
                uint gbit = (g >> (7 - bit)) & 1;
                values[bit] = (gbit) ? (g_value | (mask)) : (g_value & ~(mask)); 
                printBinary("After g value", values[bit]);   
            }
        } else if (plane == 2) {
            for (uint bit = 0; bit < 7; bit++) {
                uint32_t b_value = values[bit];
                printf("B value %d bit %d\n", b_value, bit);
                uint bbit = (b >> (7 - bit)) & 1;
                values[bit] = (bbit) ? (b_value | (mask)) : (b_value & ~(mask));  
                printBinary("After b value", values[bit]);  
            }
             
        }
        printf("After %d\n", values);
    }
    
}
// bit plane content dma channel
#define DMA_CHANNEL 0
// chain channel for configuring main dma channel to output from disjoint 8 word fragments of memory
#define DMA_CB_CHANNEL 1

#define DMA_CHANNEL_MASK (1u << DMA_CHANNEL)
#define DMA_CB_CHANNEL_MASK (1u << DMA_CB_CHANNEL)
#define DMA_CHANNELS_MASK (DMA_CHANNEL_MASK | DMA_CB_CHANNEL_MASK)

// start of each value fragment (+1 for NULL terminator)
static uintptr_t fragment_start[NUM_PIXELS * 4 + 1];

// posted when it is safe to output a new set of values
static struct semaphore reset_delay_complete_sem;
// alarm handle for handling delay
alarm_id_t reset_delay_alarm_id;

int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data) {
    reset_delay_alarm_id = 0;
    sem_release(&reset_delay_complete_sem);
    // no repeat
    return 0;
}

void __isr dma_complete_handler() {
    if (dma_hw->ints0 & DMA_CHANNEL_MASK) {
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;
        // when the dma is complete we start the reset delay timer
        if (reset_delay_alarm_id) cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
    }
}

void dma_init(PIO pio, uint sm) {
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
                          8, // 8 words for 8 bit planes
                          false);

    // chain channel sends single word pointer to start of fragment each time
    dma_channel_config chain_config = dma_channel_get_default_config(DMA_CB_CHANNEL);
    dma_channel_configure(DMA_CB_CHANNEL,
                          &chain_config,
                          &dma_channel_hw_addr(
                                  DMA_CHANNEL)->al3_read_addr_trig,  // ch DMA config (target "ring" buffer size 4) - this is (read_addr trigger)
                          NULL, // set later
                          1,
                          false);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);
}

void output_strips_dma(value_bits_t *bits, uint value_length) {
    for (uint i = 0; i < value_length; i++) {
        fragment_start[i] = (uintptr_t) bits[i].planes; // MSB first
        printBinary("Fragment start", fragment_start[i]);
    }
    fragment_start[value_length] = 0;
    dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = (uintptr_t) fragment_start;
}

void show_pixels() {
    printf("Show pixels\n");
    sem_acquire_blocking(&reset_delay_complete_sem);
    output_strips_dma(buffers[current_buffer], NUM_PIXELS * 4);
    current_buffer ^= 1;   
}
int main() {
    //set_sys_clock_48();
    stdio_init_all();
    sleep_ms(10000);
    printf("WS2812 parallel using pin %d\n", WS2812_PIN_BASE);

    PIO pio;
    uint sm;
    uint offset;
   
    for (uint pixel = 0; pixel < NUM_PIXELS; pixel++) {
        printf("Pixel %d\n", pixel);
        put_pixel(0, pixel, 0xff0000);
        put_pixel(1, pixel, 0x00ff00);
        put_pixel(2, pixel, 0x0000ff);
        
    }
    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_parallel_program, &pio, &sm, &offset, WS2812_PIN_BASE, STRIPS, true);
    hard_assert(success);

    ws2812_parallel_program_init(pio, sm, offset, WS2812_PIN_BASE, STRIPS, 800000);

    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    dma_init(pio, sm);
    memset(&buffers[0], 0, sizeof(buffers[0]));
    memset(&buffers[1], 0, sizeof(buffers[1]));

    int t = 0;
    while (1) {
        printf("Loop");
        show_pixels();
        sleep_ms(1000);
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_parallel_program, pio, sm, offset);
}
