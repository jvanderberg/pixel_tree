#ifndef DEFINES_H
#define DEFINES_H

#define MAX_RASTER_OBJECTS 100
#ifdef LOCAL_BUILD
#define NUM_PIXELS 75
#define STRIPS 16
#define BOARDS 2
#endif
#ifndef LOCAL_BUILD
#define NUM_PIXELS 140
#define STRIPS 12
#define BOARDS 2
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

#include "types.h"

extern value_bits_t colors[NUM_PIXELS * 3];
extern value_bits_t buffers[2][BOARDS][NUM_PIXELS * 3];
extern raster_object_t *raster_object[100];
extern uint current_buffer;

#endif // DEFINES_H
