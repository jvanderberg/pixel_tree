#ifndef PIXELBLIT_H
#define PIXELBLIT_H
#include "defines.h"

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