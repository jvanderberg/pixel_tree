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
#define WS2812_PIN_BASE 3

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

static uintptr_t fragment_start[NUM_PIXELS * 3 + 1];

static const uint16_t ws2812_parallel_program_instructions[] = {
    //     .wrap_target
    0x6020, //  0: out    x, 32
    0xa20b, //  1: mov    pins, !null            [2]
    0xa201, //  2: mov    pins, x                [2]
    0xa203, //  3: mov    pins, null             [2]
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program ws2812_parallel_program = {
    .instructions = ws2812_parallel_program_instructions,
    .length = 4,
    .origin = -1,
    .pio_version = ws2812_parallel_pio_version,
#if PICO_PIO_VERSION > 0
    .used_gpio_ranges = 0x0
#endif
};

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
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, pin_count, true);
    pio_sm_config c = ws2812_parallel_program_get_default_config(offset);
    sm_config_set_out_shift(&c, true, true, 32);
    sm_config_set_out_pins(&c, pin_base, pin_count);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    int cycles_per_bit = ws2812_parallel_T1 + ws2812_parallel_T2 + ws2812_parallel_T3;
    float div = clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);
    printf("div %d ", div);
    // sm_config_set_clkdiv(&c,1.5d);
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
    printf("Reset delay complete\n");
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
        sleep_us(200);
        sem_release(&reset_delay_complete_sem);
        // printf("Reset delay complete\n");
        //  if (reset_delay_alarm_id)
        //      cancel_alarm(reset_delay_alarm_id);
        //  reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
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

// DMA the current bit plane to the PIO
// We double buffer so we don't write to the memory while the DMA is reading from it
void _show_pixels_internal()
{
    for (uint board = 0; board < BOARDS; board++)
    {
        sem_acquire_blocking(&reset_delay_complete_sem);

        // Convert 'board' into a 4 bit integer and send its bits on gpio pins 0-3
        gpio_put(0, (board & 1));
        gpio_put(1, (board & 2) >> 1);
        gpio_put(2, (board & 4) >> 2);
        gpio_put(3, (board & 8) >> 3);

        output_strips_dma(buffers[current_buffer][board], NUM_PIXELS * 3);
    }

    // copy current buffer to next buffer
    memcpy(buffers[current_buffer ^ 1], buffers[current_buffer], sizeof(buffers[0]));
    // switch buffers
    current_buffer ^= 1;
    stop_timer("DMA ended");
}

void _initialize_dma()
{

    dma_init(pio, sm);

    while (1)
    {
        uint32_t task = multicore_fifo_pop_blocking(); // Wait for a command
        if (task == 1)
        {
            _show_pixels_internal(); // Execute task when received
        }
    }
}

int initialize_dma()
{
    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    // sem_init(&sending_pixels_sem, 1, 1);
    memset(&buffers[0], 0, sizeof(buffers[0]));
    memset(&buffers[1], 0, sizeof(buffers[1]));
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_parallel_program, &pio, &sm, &offset, WS2812_PIN_BASE, STRIPS, true);
    hard_assert(success);

    ws2812_parallel_program_init(pio, sm, offset, WS2812_PIN_BASE, STRIPS + WS2812_PIN_BASE, 800000);
    multicore_reset_core1();
    multicore_launch_core1(_initialize_dma);
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