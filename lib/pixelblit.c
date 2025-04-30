#include "defines.h"
#include "pixelblit.h"
#include <stdio.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/sem.h"
#include "pico/multicore.h"
#include "utils.h"

#include "pico/sem.h"

#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif
#define WS2812_PIN_BASE 4

// bit plane content dma channel
#define DMA_CHANNEL 0
// chain channel for configuring main dma channel to output from disjoint 8 word fragments of memory
#define DMA_CB_CHANNEL 1

#define DMA_CHANNEL_MASK (1u << DMA_CHANNEL)
#define DMA_CB_CHANNEL_MASK (1u << DMA_CB_CHANNEL)
#define DMA_CHANNELS_MASK (DMA_CHANNEL_MASK | DMA_CB_CHANNEL_MASK)
// Check the pin is compatible with the platform
#if WS2812_PIN_BASE >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif
int num_pixels;
int board_count;
static PIO pio;
static uint sm;
static uint offset;
static struct semaphore reset_delay_complete_sem;
static struct semaphore sending_pixels_sem;
static value_bits_t dma_board_address[BOARDS];

#define FRAGMENT_SIZE (BOARDS * (NUM_PIXELS * 3 + 1)) + 1
static uintptr_t fragment_start[FRAGMENT_SIZE]; // 3 bit planes, plus terminator, plus 1 extra for the address

#define ws2812_parallel_wrap_target 0
#define ws2812_parallel_wrap 11
#define ws2812_parallel_pio_version 0

#define ws2812_parallel_T1 3
#define ws2812_parallel_T2 3
#define ws2812_parallel_T3 4

static const uint16_t ws2812_parallel_program_instructions[] = {
    //     .wrap_target
    0x6021, //  0: out    x, 1
    0x0028, //  1: jmp    !x, 8
    0x60f0, //  2: out    exec, 16
    0x606f, //  3: out    null, 15
    0xe026, //  4: set    x, 6
    0x6060, //  5: out    null, 32
    0x0045, //  6: jmp    x--, 5
    0x0000, //  7: jmp    0
    0x603f, //  8: out    x, 31
    0xa20b, //  9: mov    pins, !null            [2]
    0xa201, // 10: mov    pins, x                [2]
    0xa003, // 11: mov    pins, null
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program ws2812_parallel_program = {
    .instructions = ws2812_parallel_program_instructions,
    .length = 12,
    .origin = -1,
    .pio_version = ws2812_parallel_pio_version,
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
#endif
};

static uint64_t start_time = 0;

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

static inline pio_sm_config ws2812_parallel_program_get_default_config(uint offset)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + ws2812_parallel_wrap_target, offset + ws2812_parallel_wrap);
    return c;
}

#include "hardware/clocks.h"
static inline void ws2812_parallel_program_init(PIO pio, uint sm, uint offset, uint pin_base, uint pin_count, float freq)
{
    for (uint i = pin_base; i < pin_base + pin_count; i++)
    {
        pio_gpio_init(pio, i);
    }
    pio_sm_config c = ws2812_parallel_program_get_default_config(offset);
    sm_config_set_out_shift(&c, true, true, 32);
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, pin_count, true);

    sm_config_set_set_pins(&c, 0, 4);
    sm_config_set_out_pins(&c, 4, STRIPS);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    int cycles_per_bit = ws2812_parallel_T1 + ws2812_parallel_T2 + ws2812_parallel_T3;
    float div = clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

#endif
// posted when it is safe to output a new set of values
static struct semaphore reset_delay_complete_sem;
// alarm handle for handling delay
alarm_id_t reset_delay_alarm_id;

int64_t reset_delay_complete(__unused alarm_id_t id, __unused void *user_data)
{
    reset_delay_alarm_id = 0;
    sem_release(&reset_delay_complete_sem);
    return 0;
}

void __isr dma_complete_handler()
{
    if (dma_hw->ints0 & DMA_CHANNEL_MASK)
    {
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;
        // when the dma is complete we start the reset delay timer
        reset_delay_alarm_id = 0;
        if (reset_delay_alarm_id)
            cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(100, reset_delay_complete, NULL, true);
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

const uint32_t one = 1;
void output_strips_dma()
{
    uint32_t position = 0;
    for (uint32_t board = 0; board < BOARDS; board++)
    {
        value_bits_t *bits = buffers[current_buffer][board];

        // set the first word of the chain channel to point to the start of the fragment
        // which encodes the board address
        fragment_start[position++] = (uintptr_t)dma_board_address[board].planes;

        for (uint i = 0; i < NUM_PIXELS * 3; i++)
        {
            fragment_start[position++] = (uintptr_t)bits[i].planes; // MSB first
        }
    }
    fragment_start[position] = 0;
    dma_channel_hw_addr(DMA_CB_CHANNEL)->al3_read_addr_trig = (uintptr_t)fragment_start;
}

// DMA the current bit plane to the PIO
// We double buffer so we don't write to the memory while the DMA is reading from it
void _show_pixels_internal()
{
    sem_acquire_blocking(&reset_delay_complete_sem);
    output_strips_dma();
    memcpy(buffers[current_buffer ^ 1], buffers[current_buffer], sizeof(buffers[0]));
    current_buffer ^= 1;
}

void _initialize_dma()
{

    for (int board = 0; board < BOARDS; board++)
    {
        // This is the PIO opcode for 'set pins, board'
        // These are flagged as special instructions in the PIO program
        // with the lowest order bit set to 1. The PIO program then loads the next 16
        // bits and executes them as a set instruction.
        //
        // We load up 8 of them in the same structure as the RGB data, because the DMA is structured to send
        // 8 words at a time. This results in small delay in the PIO program, as it reads past the 7 dummy
        // words.
        uint32_t address = 0x700 << 6 | board << 1 | 1;
        for (int i = 0; i < 8; i++)
        {
            dma_board_address[board].planes[i] = address;
        }

        printBinary("Board address:", dma_board_address[board].planes[0]);
    }

    dma_init(pio, sm);

    // while (1)
    // {
    //     uint32_t task = multicore_fifo_pop_blocking(); // Wait for a command
    //     if (task == 1)
    //     {
    //         _show_pixels_internal(); // Execute task when received
    //     }
    // }
}

int initialize_dma()
{
    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    // sem_init(&sending_pixels_sem, 1, 1);
    memset(&buffers[0], 0, sizeof(buffers[0]));
    memset(&buffers[1], 0, sizeof(buffers[1]));
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_parallel_program, &pio, &sm, &offset, 0, STRIPS + 4, false);
    hard_assert(success);

    ws2812_parallel_program_init(pio, sm, offset, 0, STRIPS + 4, 800000);
    // multicore_reset_core1();
    _initialize_dma();
}
int remove_dma()
{
    pio_remove_program_and_unclaim_sm(&ws2812_parallel_program, pio, sm, offset);
}
// start of each value (+1 for NULL terminator)
value_bits_t colors[NUM_PIXELS * 3];
// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version

// posted when it is safe to output a new set of values
static struct semaphore reset_delay_complete_sem;
static struct semaphore sending_pixels_sem;

// alarm handle for handling delay
alarm_id_t reset_delay_alarm_id;

void show_pixels()
{
    multicore_fifo_push_blocking(1);
}

uint count = 0;

void show_pixels_with_refresh_rate(uint frequency)
{
    if (start_time == 0)
    {
        start_time = time_us_64();
    }
    uint ms = 1000000 / frequency;
    uint64_t elapsed = time_us_64() - start_time;

    if (count % 100 == 0)
    {
        printf("ms: %d %d %d\n", ms, elapsed, count);
    }

    count = count + 1;
    // printf("ms: %d %d\n", ms, elapsed);

    if (elapsed < ms)
    {
        sleep_us(ms - elapsed - 200);
    }
    else
    {
        // sleep_ms(1000);
    }
    elapsed = time_us_64() - start_time;
    start_time = time_us_64();

    _show_pixels_internal();
    // multicore_fifo_push_blocking(1);
}
#define FIXED_SHIFT 10

// uint64_t elapsed_time = 0;
static uint64_t starting_time = 0;
uint64_t countp = 0;
uint64_t animate(float start, float pixel_per_second, int size)
{
    if (starting_time == 0)
    {
        printf("Reset starting time!!!!!");
        starting_time = time_us_64();
    }
    uint64_t temp_starting_time = starting_time;
    uint64_t int_pixel_per_second = ((uint64_t)(pixel_per_second * 65536));
    uint64_t elapsed_time = time_us_64() - temp_starting_time; // + 31556952000000;
    uint64_t int_shift = ((elapsed_time * int_pixel_per_second)) / 1000000;

    // printf("Elapsed int time: %llu %llu\n", starting_time, int_shift);
    // float shift = fmodf((float)int_shift / 65536 / 1000000, 1.0f);
    // countp++;
    // printf("Elapsed time: %llu %f\n", elapsed_time, shift);

    // printf("Elapsed time: %f %llu %llu\n", pixel_per_second, elapsed_time, int_shift);

    return int_shift;
}

int16_t fixed_cos(int angle)
{
    return (int16_t)(cos(angle * M_PI / 180.0) * (1 << FIXED_SHIFT));
}

int16_t fixed_sin(int angle)
{
    return (int16_t)(sin(angle * M_PI / 180.0) * (1 << FIXED_SHIFT));
}

// Blends two colors with an alpha weight (fixed-point alpha)
uint8_t blend_pixel(int alpha, uint8_t base, uint8_t color)
{
    return (uint8_t)(((1024 - alpha) * base + alpha * color) >> FIXED_SHIFT);
}

// Checks if a point is inside the rotated rectangle with rounded corners
int point_in_rounded_rectangle(int px, int py, int cx, int cy, int w, int h, int cos_theta, int sin_theta, int radius)
{
    // Translate point to rectangle center
    int dx = px - cx;
    int dy = py - cy;

    // Rotate point (fixed-point math)
    int rx = ((dx * cos_theta) - (dy * sin_theta)) >> FIXED_SHIFT;
    int ry = ((dx * sin_theta) + (dy * cos_theta)) >> FIXED_SHIFT;

    // Check if inside rectangle bounds
    if (abs(rx) <= (w / 2) && abs(ry) <= (h / 2))
    {
        return 1;
    }

    // Check if inside rounded corners (distance from corner)
    int corner_x = (rx > 0 ? (w / 2) : -(w / 2));
    int corner_y = (ry > 0 ? (h / 2) : -(h / 2));

    int dist_sq = (rx - corner_x) * (rx - corner_x) + (ry - corner_y) * (ry - corner_y);
    return dist_sq <= (radius * radius);
}

// Draws an anti-aliased rotated rectangle with rounded corners (fixed-point math)
void draw_rectangle(int raster, int x, int y, int w, int h, int r, uint32_t c, int radius)
{
    raster_object_t ro = get_raster(raster);
    // Compute half-width and half-height
    int half_w = (w << (FIXED_SHIFT - 1)) >> FIXED_SHIFT;
    int half_h = (h << (FIXED_SHIFT - 1)) >> FIXED_SHIFT;

    int min_x = x - half_w;
    int max_x = x + half_w;
    int min_y = y - half_h;
    int max_y = y + half_h;

    // printf("Min x: %d, Max x: %d, Min y: %d, Max y: %d\n", min_x, max_x, min_y, max_y);
    int samples = 4;                     // 2x2 Supersampling
    int sub_pixel_step = 1024 / samples; // Fixed-point step

    // Precompute cosine and sine (fixed-point)
    int cos_theta = fixed_cos(r);
    int sin_theta = fixed_sin(r);

    for (int py = min_y; py <= max_y; py++)
    {
        for (int px = min_x; px <= max_x; px++)
        {
            int coverage = 0;

            // Supersampling loop
            for (int sy = 0; sy < samples; sy++)
            {
                for (int sx = 0; sx < samples; sx++)
                {
                    int sample_x = (px << FIXED_SHIFT) + ((sx * sub_pixel_step) + (sub_pixel_step / 2));
                    int sample_y = (py << FIXED_SHIFT) + ((sy * sub_pixel_step) + (sub_pixel_step / 2));

                    if (point_in_rounded_rectangle(sample_x >> FIXED_SHIFT, sample_y >> FIXED_SHIFT, x, y, w, h, cos_theta, sin_theta, radius))
                    {
                        coverage += 1;
                    }
                }
            }

            coverage = (coverage * 1024) / (samples * samples); // Normalize to 1024

            if (coverage > 0)
            {
                int clamped_x = (px < 0) ? 0 : (px >= ro.width ? ro.width - 1 : px);
                int clamped_y = (py < 0) ? 0 : (py >= ro.height ? ro.height - 1 : py);

                // Blend color based on coverage
                ro.raster[clamped_y][clamped_x] = c;
            }
        }
    }
}
