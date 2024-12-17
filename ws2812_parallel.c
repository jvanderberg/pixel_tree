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
#include <math.h>
#include <float.h>

#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"

// The number of pixels in the 'real' strip, this is the physical addressing
#define NATIVE_NUM_PIXELS 88
#define WS2812_PIN_BASE 0
// The number of real strips. As a strip might be split into multiple effective strips in the display
#define NATIVE_STRIPS 12

// The number of pixels in the display, this is the logical addressing, as a single strip might
// wrap around to become a different strip in the reverse direciton
#define NUM_PIXELS 42
#define STRIPS 24
// Just a simple raster display of RGB values for each pixel
uint32_t display[STRIPS][NUM_PIXELS];

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

static value_bits_t colors[NATIVE_NUM_PIXELS * 3];
// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version

uint current_buffer = 0;
static value_bits_t buffers[2][NATIVE_NUM_PIXELS * 3];

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
static uintptr_t fragment_start[NATIVE_NUM_PIXELS * 3 + 1];

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

    for (int i = 0; i < NATIVE_NUM_PIXELS * 3; i++)
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
    output_strips_dma(buffers[current_buffer], NATIVE_NUM_PIXELS * 3);
    // copy current buffer to next buffer
    memcpy(buffers[current_buffer ^ 1], buffers[current_buffer], sizeof(buffers[0]));
    // switch buffers
    current_buffer ^= 1;
}

void get_physical_pixel_address(int xy[2])
{
    int strip = xy[0];
    int pixel = xy[1];

    int native_strip = strip / 2;
    int native_pixel = pixel;
    if (strip % 2)
    {

        native_pixel = 2 * NUM_PIXELS - pixel - 1;
    }

    xy[0] = native_strip;
    xy[1] = native_pixel + 1;
}

// Copy pixels from display to bitplanes using put_pixel
void show_display()
{
    for (uint strip = 0; strip < STRIPS; strip++)
    {

        for (uint pixel = 0; pixel < NUM_PIXELS; pixel++)
        {
            int xy[2] = {strip, pixel};
            get_physical_pixel_address(xy);
            put_pixel(xy[0], xy[1], display[strip][pixel]);
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

// HSL to RGB
uint32_t hsl_to_rgb(float h, float s, float l)
{

    float c = (1 - fabs(2 * l - 1)) * s;
    float x = c * (1 - fabs(fmod(h * 6, 2) - 1));
    float m = l - c / 2;
    float r, g, b;
    if (h < 1.0 / 6.0)
    {
        r = c;
        g = x;
        b = 0;
    }
    else if (h < 2.0 / 6.0)
    {
        r = x;
        g = c;
        b = 0;
    }
    else if (h < 3.0 / 6.0)
    {
        r = 0;
        g = c;
        b = x;
    }
    else if (h < 4.0 / 6.0)
    {
        r = 0;
        g = x;
        b = c;
    }
    else if (h < 5.0 / 6.0)
    {
        r = x;
        g = 0;
        b = c;
    }
    else
    {
        r = c;
        g = 0;
        b = x;
    }
    uint8_t red = (r + m) * 255;
    uint8_t green = (g + m) * 255;
    uint8_t blue = (b + m) * 255;
    return (red << 16) | (green << 8) | blue;
}

// RGB to HSL
void rgb_to_hsl(uint32_t rgb, float *h, float *s, float *l)
{
    float r = ((rgb >> 16) & 0xff) / 255.0;
    float g = ((rgb >> 8) & 0xff) / 255.0;
    float b = (rgb & 0xff) / 255.0;
    float max = fmax(r, fmax(g, b));
    float min = fmin(r, fmin(g, b));
    *l = (max + min) / 2;
    if (max == min)
    {
        *h = 0;
        *s = 0;
    }
    else
    {
        float d = max - min;
        *s = *l > 0.5 ? d / (2 - max - min) : d / (max + min);
        if (max == r)
        {
            *h = (g - b) / d + (g < b ? 6 : 0);
        }
        else if (max == g)
        {
            *h = (b - r) / d + 2;
        }
        else
        {
            *h = (r - g) / d + 4;
        }
        *h /= 6;
    }
}
uint64_t last_rainbow_time = 0;

// Define a type for the scheduled functions
typedef void (*ScheduledFunction)(void *);

// Struct to hold function details
typedef struct
{
    ScheduledFunction function;
    void *param;
    uint32_t time_seconds;
} FunctionSchedule;

uint64_t start_time = 0;
size_t current_index = 0;
// Function to execute scheduled tasks
void run_scheduler(FunctionSchedule *schedule, size_t count)
{

    float elapsed_time = (time_us_64() - start_time) / 1000000; // Convert to seconds
    // Check if params are not null
    if (schedule[current_index].param != NULL)
    {
        // Call the function with the params
        schedule[current_index].function(schedule[current_index].param);
    }
    else
    { // Call the the function without parameters

        schedule[current_index].function(NULL);
    }

    // Check if the time for the current function has expired
    if (elapsed_time >= schedule[current_index].time_seconds)
    {

        // Reset the timer for this function and move to the next
        start_time = time_us_64();
        current_index = (current_index + 1) % count; // Loop back to the first function
    }
}

void rainbow()
{
    float h = 0;
    float s = 1;
    float l = 0.5;

    // Run every 16ms
    if (time_us_64() - last_rainbow_time < 16000)
    {
        return;
    }
    else
    {
        last_rainbow_time = time_us_64();
    }

    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            uint32_t rgb = display[i][j];
            float h, s2, l2;
            rgb_to_hsl(rgb, &h, &s2, &l2);
            h += 0.01;
            if (h > 1)
            {
                h -= 1;
            }
            uint32_t new_rgb = hsl_to_rgb(h, s, l);
            display[i][j] = new_rgb;
        }
    }
}

int current_strip = 0;

void diag()
{
    float h = 0;
    float s = 1;
    float l = 0.5;

    // Run every 16ms
    if (time_us_64() - last_rainbow_time < 250000)
    {
        return;
    }
    else
    {
        last_rainbow_time = time_us_64();
    }

    current_strip = (current_strip + 1) % STRIPS;
    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            if (i == current_strip)
            {
                display[i][j] = 0x0000ff;
            }
            else
            {
                display[i][j] = 0x000000;
            }
        }
    }
}
void set_string(uint strip, uint32_t color)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        display[strip][i] = color;
    }
}

void init_rainbow(void *param)
{
    float h = 0;
    float s = 1;
    float l = 0.5;
    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            h = (float)j / NUM_PIXELS;
            h = h + (float)i / STRIPS;
            // h = (float)i / STRIPS;
            uint32_t new_rgb = hsl_to_rgb(h, s, l);
            display[i][j] = new_rgb;
            // display[i][j] = 0xaaaaaa;
        }
    }
}

uint32_t fade_rgb(uint32_t rgb, uint8_t fade)
{
    // Extract individual color channels
    uint8_t red = (rgb >> 16u) & 0xFF;  // Extract red (upper 8 bits)
    uint8_t green = (rgb >> 8u) & 0xFF; // Extract green (middle 8 bits)
    uint8_t blue = rgb & 0xFF;          // Extract blue (lower 8 bits)

    // Scale each channel using the fade factor
    red = (red * fade) >> 8u;
    green = (green * fade) >> 8u;
    blue = (blue * fade) >> 8u;
    // printf("Fade %d\n", fade);

    // Combine the faded color channels back into a 24-bit integer
    return (red << 16u) | (green << 8u) | blue;
}
// Fade the display by a certain amount
// amount is a value between 0 and 255, 255 is no fade
void fade(void *param)
{
    // printf("Fading\n %d", param);
    uint amount = (param == NULL) ? 240 : *(uint8_t *)param;
    // printf("Fade in fade %d\n", amount);
    float h = 0;
    float s = 1;
    float l = 0.5;
    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            uint32_t rgb = display[i][j];
            display[i][j] = fade_rgb(rgb, amount);
        }
    }
}

void run_sparkle(void *param)
{
    uint32_t color = *(uint32_t *)param;
    // Run every 16ms
    if (time_us_64() - last_sparkle_time < 16000)
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
            // Randomly every 200th time reset the pixel to white
            if (rand() % 100 == 0)
            {
                display[i][j] = color;
            }
            else
            {
                display[i][j] = fade_rgb(rgb, 245u);
            }
        }
    }
}

uint64_t last_shooting_star_time = 0;

int star_pixels[STRIPS] = {0};
void init_shooting_star(void *param)
{
    for (int i = 0; i < STRIPS; i++)
    {
        star_pixels[i] = rand() % NUM_PIXELS;
    }
}

void shooting_star(void *param)
{
    uint fade_amount = 250;
    fade(&fade_amount);
    uint32_t color = *(uint32_t *)param;
    // Run every 16ms
    if (time_us_64() - last_shooting_star_time < 32000)
    {
        return;
    }
    else
    {
        last_shooting_star_time = time_us_64();
    }
    if (rand() % 100 == 0)
    {

        star_pixels[rand() % STRIPS] = rand() % NUM_PIXELS;
    }

    for (int i = 0; i < STRIPS; i++)
    {
        int pos = star_pixels[i];
        if (pos > 0)
        {
            display[i][pos] = color;
            star_pixels[i]--;
        }
        else
        {
            star_pixels[i] = NUM_PIXELS - 1;
        }
    }
}

int spiral_location[2] = {0, 0};
int spiral_location2[2] = {0, 0};

void init_spiral(void *param)
{
    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            display[i][j] = 0x000000;
        }
    }
    spiral_location[0] = 0;
    spiral_location[1] = 0;
    spiral_location2[0] = 12;
    spiral_location2[1] = 1;
}

void spiral(void *param)
{

    uint32_t color = *(uint32_t *)param;
    if (time_us_64() - last_sparkle_time < 1000)
    {
        return;
    }
    else
    {
        last_sparkle_time = time_us_64();
    }
    int fade_amount = 254;
    fade(&fade_amount);
    spiral_location[0] = (spiral_location[0] + 1);
    if (spiral_location[0] >= STRIPS)
    {
        spiral_location[0] = 0;
        spiral_location[1] = (spiral_location[1] + 3) % NUM_PIXELS;
    }
    display[spiral_location[0]][spiral_location[1]] = color;
    spiral_location2[0] = (spiral_location2[0] + 1);
    if (spiral_location2[0] >= STRIPS)
    {
        spiral_location2[0] = 0;
        spiral_location2[1] = (spiral_location2[1] + 3) % NUM_PIXELS;
    }
    display[spiral_location2[0]][spiral_location2[1]] = 0x00ff00;
}

uint32_t four_color_palette[6] = {0xff0000, 0x00ff00, 0x0000ff, 0x00ffff, 0xffff00, 0xff00ff};
uint32_t last_fourcolor_time = 0;

void four_color(void *param)
{
    if (time_us_64() - last_fourcolor_time < 2000000)
    {
        return;
    }
    else
    {
        last_fourcolor_time = time_us_64();
    }

    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            // Put 4 colors on the display somewhat randomly
            display[i][j] = four_color_palette[rand() % 6];
        }
    }
}

void solid_color(void *param)
{
    uint32_t color = *(uint32_t *)param;
    for (int i = 0; i < STRIPS; i++)
    {
        for (int j = 0; j < NUM_PIXELS; j++)
        {
            display[i][j] = color;
        }
    }
}
uint32_t sparkle_color = 0x0000ff;
uint32_t white = 0xffffff;
uint32_t black = 0x000000;
uint32_t red = 0xff0000;
uint32_t green = 0x00ff00;
uint32_t blue = 0x0000ff;
uint fade_slow = 253;
int fade_millis = 50;
FunctionSchedule schedule[] = {
    {init_spiral, NULL, 1},
    {spiral, &red, 60},
    {fade, &fade_slow, 2},
    {four_color, NULL, 60},
    {solid_color, &green, 60},     // Call every second
    {init_shooting_star, NULL, 1}, // Call every second
    {shooting_star, &white, 60},   // Call every 2 seconds
    {fade, &fade_slow, 2},         // Call every 3 seconds
    {run_sparkle, &white, 60},     // Call every 2 seconds
    {fade, &fade_slow, 2},         // Call every 3 seconds
    {init_rainbow, NULL, 1},       // Call every second
    {rainbow, NULL, 60},           // Call every 3 seconds
    {fade, &fade_slow, 2},
    {init_shooting_star, NULL, 1},
    {shooting_star, &blue, 60},
    {fade, &fade_slow, 3}, // Call every 2 seconds
    {solid_color, &red, 60},
    {run_sparkle, &blue, 60},
    {spiral, &white, 60},
    {fade, &fade_slow, 2},
    {init_shooting_star, NULL, 1},
    {shooting_star, &green, 60},
    {solid_color, &blue, 60}};

uint64_t my_timer;
int main()
{
    start_time = time_us_64();
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
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_parallel_program, &pio, &sm, &offset, WS2812_PIN_BASE, NATIVE_STRIPS, true);
    hard_assert(success);

    ws2812_parallel_program_init(pio, sm, offset, WS2812_PIN_BASE, NATIVE_STRIPS, 800000);

    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    dma_init(pio, sm);
    init_rainbow(NULL);
    // int amount = 253;
    init_spiral(NULL);
    while (1)
    {

        run_scheduler(schedule, sizeof(schedule) / sizeof(schedule[0]));
        show_pixels();
        show_display();
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_parallel_program, pio, sm, offset);
}
