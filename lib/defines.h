#ifndef DEFINES_H
#define DEFINES_H
#define NUM_PIXELS 100
#define STRIPS 16
#define BOARDS 10
#define MAX_RASTER_OBJECTS 100
#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif
#ifndef LOCAL_BUILD
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/sem.h"
#include <stdint.h>
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
#endif

typedef enum
{
    CLIP = 0,
    NO_WRAP = 1,
    WRAP = 2,
} WrapMode;
#define VALUE_PLANE_COUNT (8)
typedef struct
{
    // stored MSB first
    uint32_t planes[VALUE_PLANE_COUNT];
} value_bits_t;

typedef struct
{
    uint8_t board;
    uint8_t strip;
    uint8_t pixel;
} pixel_address_t;

typedef struct
{
    uint16_t height;
    uint16_t width;
    uint32_t **raster;
    pixel_address_t **pixel_mapping;
} raster_object_t;
extern value_bits_t colors[NUM_PIXELS * 3];

// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version
extern value_bits_t buffers[2][BOARDS][NUM_PIXELS * 3];
extern raster_object_t *raster_object[100];
extern uint current_buffer;

#endif // DEFINES_H