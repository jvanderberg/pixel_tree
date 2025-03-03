#ifndef PIXELBLIT_H
#define PIXELBLIT_H
#include "defines.h"
// --------------- //
// ws2812_parallel //
// --------------- //

#define ws2812_parallel_wrap_target 0
#define ws2812_parallel_wrap 3
#define ws2812_parallel_pio_version 0

#define ws2812_parallel_T1 3
#define ws2812_parallel_T2 3
#define ws2812_parallel_T3 4
#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif
// Function prototypes
int initialize_dma();

int remove_dma();

void show_pixels();

#endif // PIXELBLIT_H