#include "defines.h"
#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif
#include "pixelblit.h"
// Create a direct raster object. starting at the given board, strip, and pixel
// This is useful for creating a raster object that is a subset of the display
// This simplistically just tries to map a width x height grid, just advancing first the pixel,
// then the strip, then the board, until it finds width x height pixels
// If WrapModes is CLIP, and width is less than the number of pixels on a strip, it will just stop collecting pixel from
// that strip, and advance to the next string.
// If WrapMode is NO_WRAP and width is less than the number of pixels on a strip, it will advance to the next row in the raster,
// and start taking pixels from the next strip
// If WrapMode is WRAP, and width is less than the number of pixels on a strip, but alternate the order of the mapping of the pixels, reversing every other row.
// This is useful if for example, you have a string of 100 pixels, and you zig/zag it 4 timess to
// create 4, 25 pixel strips. WrapMode WRAP will map the pixels in the order 0-24, 49-25, 50-74, 99-75

// Height is the strip number, width is the pixel number.
// Boards are assumed to have 16 strips and NUM_PIXELS pixels on each strip
// If this isn't true, just ensure that NUM_PIXELS is the number of pixels on the longest strip
//

int create_raster(uint16_t height, uint16_t width, uint board, uint strip, uint pixel, WrapMode wrap);

raster_object_t get_raster(uint raster_id);

void show_all_raster_objects();

void show_raster_object(int i);
void show_raster_object_with_shift(int i, float shift_x, float shift_y);

void draw_pixel(int raster_id, int x, int y, uint32_t color);

void fill_raster(int raster_id, uint32_t color);

void fade_raster(uint raster_index, uint8_t amount);

// HSL to RGB
uint32_t hsl_to_rgb(float h, float s, float l);
// RGB to HSL
void rgb_to_hsl(uint32_t rgb, float *h, float *s, float *l);

void rainbow(int raster_id);

void init_rainbow(int raster_id);