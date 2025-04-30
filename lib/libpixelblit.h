#ifndef LIBPIXELBLIT_H
#define LIBPIXELBLIT_H

#include "defines.h"
#include "types.h"

// Function prototypes
int initialize_dma();

int remove_dma();

void show_pixels();
void show_pixels_with_refresh_rate(uint frequency);
uint64_t animate(float start, float pixel_per_second, int pixels);
void draw_rectangle(int raster, int x, int y, int w, int h, int r, uint32_t c, int radius);

#endif // LIBPIXELBLIT_H