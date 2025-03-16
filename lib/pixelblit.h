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
void show_pixels_with_refresh_rate(uint frequency);
uint64_t animate(float start, float pixel_per_second, int pixels);
void draw_rectangle(int raster, int x, int y, int w, int h, int r, uint32_t c, int radius);

#endif // PIXELBLIT_H