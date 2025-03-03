#include "defines.h"
#include <stdlib.h> // Required for malloc
#include <stdio.h>
#include "utils.h"
#include <math.h>
#include <float.h>
#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

void show_pixels()
{
    // stub
}
#endif
#ifndef LOCAL_BUILD
#include "pixelblit.h"
#include "utils.h"
#endif

uint current_buffer = 0;
// Up to 100 raster objects
raster_object_t *raster_object[MAX_RASTER_OBJECTS];

// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version
value_bits_t buffers[2][BOARDS][NUM_PIXELS * 3];

int raster_object_count = -1;

int create_raster(uint16_t height, uint16_t width, uint board, uint strip, uint pixel, WrapMode wrap)

{

    raster_object_count++;
    if (raster_object_count >= MAX_RASTER_OBJECTS)
    {
        printf("Max raster objects reached, not creating new raster object\n");
        return -1;
    }
    printf("Creating raster object %d\n", raster_object_count);
    uint offset = pixel;

    raster_object_t *raster = malloc(sizeof(raster_object_t));
    printf("Height: %d, Width: %d\n", height, width);
    raster->height = height;
    raster->width = width;
    uint32_t *raster_data = malloc(height * width * sizeof(uint32_t));

    pixel_address_t *pixel_mapping_data = malloc(height * width * sizeof(pixel_address_t));

    raster->raster = malloc(height * sizeof(uint32_t *));
    if (!raster->raster)
    {
        perror("Failed to allocate row pointers");
        return 1;
    }
    raster->pixel_mapping = malloc(height * sizeof(pixel_address_t *));
    for (int i = 0; i < height; i++)
    {

        raster->raster[i] = raster_data + i * width;
        raster->pixel_mapping[i] = pixel_mapping_data + i * width;
    }

    uint wrap_width = 0;
    uint current_wrap = 0;

    if (wrap == WRAP)
    {
        if (NUM_PIXELS % width != 0)
        {
            printf("Width does not evenly divide NUM_PIXELS, WRAP mode disabled, NO_WRAP defaulting\n");
        }
        else
        {
            wrap_width = NUM_PIXELS / width;
        }
    }
    current_wrap = 0;
    for (int i = 0; i < height; i++)
    {

        for (int j = 0; j < width; j++)
        {
            raster->raster[i][j] = 0;
            raster->pixel_mapping[i][j].board = board;
            raster->pixel_mapping[i][j].strip = strip;
            raster->pixel_mapping[i][j].pixel = offset;

            if (j == 0)
            {
                switch (wrap)
                {
                case NO_WRAP:
                    current_wrap = 0;
                    break;
                case WRAP:

                    current_wrap++;
                    break;
                default:
                    break;
                }
            }

            if (wrap == WRAP)
            {

                // if the current wrap is odd, then use pixel, if it's even, move down from the next wrap width
                if (current_wrap % 2 == 0)
                {
                    offset = (width * (current_wrap + 1)) - pixel - 1;
                }
                else
                {
                    offset = pixel;
                }
            }
            else if (wrap == CLIP)
            {

                if (pixel >= width)
                {
                    pixel = 0;
                    current_wrap = 0;
                    strip++;
                    if (strip >= STRIPS)
                    {
                        strip = 0;
                        board++;
                        if (board >= BOARDS)
                        {
                            board = 0;
                        }
                    }
                }
                offset = pixel;
            }
            else
            {
                offset = pixel;
            }

            raster->raster[i][j] = 0;
            raster->pixel_mapping[i][j].board = board;
            raster->pixel_mapping[i][j].strip = strip;
            raster->pixel_mapping[i][j].pixel = offset;

            pixel++;
            if (pixel >= NUM_PIXELS)
            {
                pixel = 0;
                current_wrap = 0;
                strip++;
                if (strip >= STRIPS)
                {
                    strip = 0;
                    board++;
                    if (board >= BOARDS)
                    {
                        board = 0;
                    }
                }
            }
        }
    }
    raster_object[raster_object_count] = raster;
    return raster_object_count;
}

raster_object_t get_raster(uint raster_id)
{
    if (raster_id <= raster_object_count)
    {
        return *raster_object[raster_id];
    }
    else
    {
        printf("Invalid raster id\n");
        raster_object_t empty;
        empty.height = 0;
        empty.width = 0;
        empty.raster = NULL;
        empty.pixel_mapping = NULL;
        return empty;
    }
}

// Move all raster objects to the display buffer, and write to the strings
void show_all_raster_objects()
{
    for (int i = 0; i <= raster_object_count; i++)
    {
        show_raster_object(i);
    }
    show_pixels();
}

/**
 * Put a pixel into the bit plane buffer
 */
void put_pixel(uint board, uint strip, uint pixel, uint32_t pixel_rgb)
{

    uint b = pixel_rgb & 0xffu;
    uint g = (pixel_rgb >> 8u) & 0xffu;
    uint r = (pixel_rgb >> 16u) & 0xffu;
    uint v = pixel * 3;

    uint32_t mask = 1 << (strip + 1); // The mask for the current strip

    uint color_array[3] = {r, g, b};

    // Iterate through the colors
    for (int i = 0; i < 3; i++)
    { // Each bit plane is 32 bits, one bit for each strip, with the MSB being the first strip
        // There are three bit planes, one for each color
        uint32_t *values = buffers[current_buffer][board][v + i].planes;
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

void draw_pixel(int raster_id, int x, int y, uint32_t color)
{
    raster_object_t raster = get_raster(raster_id);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in draw_pixel: %i\n", raster_id);
        return;
    }
    uint index = y * raster.width + x;
    raster.raster[x][y] = color;
}

void fill_raster(int raster_id, uint32_t color)
{
    raster_object_t raster = get_raster(raster_id);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in fill_raster: %i\n", raster_id);
        return;
    }
    for (int i = 0; i < raster.height; i++)
    {
        for (int j = 0; j < raster.width; j++)
        {
            raster.raster[i][j] = color;
        }
    }
}

void show_raster_object(int i)
{
    raster_object_t raster = get_raster(i);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in put_raster_object: %i\n", i);
        return;
    }
    for (int j = 0; j < raster.height; j++)
    {
        for (int k = 0; k < raster.width; k++)
        {

            uint32_t color = raster.raster[j][k];
            put_pixel(raster.pixel_mapping[j][k].board, raster.pixel_mapping[j][k].strip, raster.pixel_mapping[j][k].pixel, color);
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
// Fade the raster
// amount is a value between 0 and 255, 255 is min fade, 0 is full fade
void fade_raster(uint raster_index, uint8_t amount)
{
    raster_object_t *raster = raster_object[raster_index];
    for (int i = 0; i < raster->height; i++)
    {
        for (int j = 0; j < raster->width; j++)
        {
            uint32_t rgb = raster->raster[i][j];
            raster->raster[i][j] = fade_rgb(rgb, amount);
        }
    }
}

uint32_t hsl_to_rgb(float h, float s, float l)
{
    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float h6 = h * 6.0f;
    float x = c * (1.0f - fabsf(fmodf(h6, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r = 0, g = 0, b = 0;
    int region = (int)h6;

    switch (region)
    {
    case 0:
        r = c;
        g = x;
        b = 0;
        break;
    case 1:
        r = x;
        g = c;
        b = 0;
        break;
    case 2:
        r = 0;
        g = c;
        b = x;
        break;
    case 3:
        r = 0;
        g = x;
        b = c;
        break;
    case 4:
        r = x;
        g = 0;
        b = c;
        break;
    default:
        r = c;
        g = 0;
        b = x;
        break;
    }

    uint8_t red = (uint8_t)((r + m) * 255.0f);
    uint8_t green = (uint8_t)((g + m) * 255.0f);
    uint8_t blue = (uint8_t)((b + m) * 255.0f);

    return (red << 16) | (green << 8) | blue;
}
// RGB to HSL
void rgb_to_hsl(uint32_t rgb, float *h, float *s, float *l)
{
    // Extract and normalize RGB components
    float r = ((rgb >> 16) & 0xFF) * (1.0f / 255.0f);
    float g = ((rgb >> 8) & 0xFF) * (1.0f / 255.0f);
    float b = (rgb & 0xFF) * (1.0f / 255.0f);

    // Compute min, max, and lightness
    float max = fmaxf(r, fmaxf(g, b));
    float min = fminf(r, fminf(g, b));
    float d = max - min;
    *l = (max + min) * 0.5f;

    if (d == 0)
    {
        *h = 0.0f; // Undefined hue for grayscale colors
        *s = 0.0f;
        return;
    }

    // Compute saturation
    *s = (*l > 0.5f) ? (d / (2.0f - max - min)) : (d / (max + min));

    // Compute hue
    float hue;
    if (max == r)
    {
        hue = (g - b) / d + (g < b ? 6.0f : 0.0f);
    }
    else if (max == g)
    {
        hue = (b - r) / d + 2.0f;
    }
    else
    {
        hue = (r - g) / d + 4.0f;
    }

    *h = hue * (1.0f / 6.0f);
}

uint32_t mix_rgb(uint32_t rgb1, uint32_t rgb2, float amount)
{
    // Extract and normalize RGB values
    float r1 = ((rgb1 >> 16) & 0xFF) * (1.0f / 255.0f);
    float g1 = ((rgb1 >> 8) & 0xFF) * (1.0f / 255.0f);
    float b1 = (rgb1 & 0xFF) * (1.0f / 255.0f);

    float r2 = ((rgb2 >> 16) & 0xFF) * (1.0f / 255.0f);
    float g2 = ((rgb2 >> 8) & 0xFF) * (1.0f / 255.0f);
    float b2 = (rgb2 & 0xFF) * (1.0f / 255.0f);

    // Compute brightness before blending
    float brightness1 = fmaxf(r1, fmaxf(g1, b1));
    float brightness2 = fmaxf(r2, fmaxf(g2, b2));

    // Linear interpolation (LERP)
    float r = r1 * (1.0f - amount) + r2 * amount;
    float g = g1 * (1.0f - amount) + g2 * amount;
    float b = b1 * (1.0f - amount) + b2 * amount;

    // Compute new brightness
    float new_brightness = fmaxf(r, fmaxf(g, b));
    float original_brightness = fmaxf(brightness1, brightness2);

    // Preserve brightness if it's decreasing
    if (new_brightness > 0.0f && new_brightness < original_brightness)
    {
        float scale = original_brightness / new_brightness;
        r *= scale;
        g *= scale;
        b *= scale;
    }

    // Clamp values to prevent overflow
    r = fminf(fmaxf(r, 0.0f), 1.0f);
    g = fminf(fmaxf(g, 0.0f), 1.0f);
    b = fminf(fmaxf(b, 0.0f), 1.0f);

    // Convert back to uint32_t
    return ((uint8_t)(r * 255) << 16) | ((uint8_t)(g * 255) << 8) | (uint8_t)(b * 255);
}
void rainbow(int raster_id)
{

    raster_object_t raster = get_raster(raster_id);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in rainbow: %i\n", raster_id);
        return;
    }

    // for (uint i = 0; i < raster.height; i++)
    // {
    //     for (uint j = 0; j < raster.width; j++)
    //     {
    //         uint32_t rgb = raster.raster[i][j];
    //         float h, s, l;
    //         rgb_to_hsl(rgb, &h, &s, &l);
    //         //  printf("HSL: %d %d %d\n", h, s2, l2);
    //         h += 0.01;
    //         if (h > 1)
    //         {
    //             h -= 1;
    //         }
    //         uint32_t new_rgb = hsl_to_rgb(h, s, l);
    //         // printBinary("New RGB", new_rgb);
    //         raster.raster[i][j] = new_rgb;
    //     }
    // }
    // uint32_t *temp = malloc(raster.width * sizeof(uint32_t));

    for (uint i = 0; i < raster.height; i++)
    {
        uint32_t save = raster.raster[i][0];
        for (uint j = 0; j < raster.width - 1; j++)
        {

            raster.raster[i][j] = mix_rgb(raster.raster[i][j], raster.raster[i][j + 1], 0.5);

            // raster.raster[i][j] = fade_rgb(raster.raster[i][j], 128) + fade_rgb(raster.raster[i][j + 1], 128);
        }
        raster.raster[i][raster.width - 1] = mix_rgb(save, raster.raster[i][raster.width - 1], 0.5);
    }
}

void init_rainbow(int raster_id)
{
    float h = 0;
    float s = 1;
    float l = 0.5;

    raster_object_t raster = get_raster(raster_id);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in rainbow: %i\n", raster_id);
        return;
    }
    for (uint board = 0; board < BOARDS; board++)
    {
        for (uint i = 0; i < raster.height; i++)
        {
            for (uint j = 0; j < raster.width; j++)
            {

                h = (float)j / raster.width;
                h = h + (float)i / raster.height;
                if (h > 1)
                {
                    h -= 1;
                }
                uint32_t new_rgb = hsl_to_rgb(h, s, l);
                raster.raster[i][j] = new_rgb;
            }
        }
    }
}

// Fast integer-based bilinear interpolation (16-bit precision)
static inline uint32_t bilinear_interpolate(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11, int fx, int fy)
{
    // Extract 8-bit RGB components
    int r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
    int r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
    int r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
    int r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

    // Compute bilinear interpolation weights (fixed-point 16-bit)
    int w00 = ((uint32_t)(65536 - fx) * (65536 - fy)) >> 16;
    int w10 = ((uint32_t)fx * (65536 - fy)) >> 16;
    int w01 = ((uint32_t)(65536 - fx) * fy) >> 16;
    int w11 = ((uint32_t)fx * fy) >> 16;

    // printf("Weights: %d %d %d %d %d %d\n", w00, w10, w01, w10, fx, fy);

    // Compute interpolated RGB values
    int r = (r00 * w00 + r10 * w10 + r01 * w01 + r11 * w11) >> 16;
    int g = (g00 * w00 + g10 * w10 + g01 * w01 + g11 * w11) >> 16;
    int b = (b00 * w00 + b10 * w10 + b01 * w01 + b11 * w11) >> 16;

    // Pack back into uint32_t
    return (r << 16) | (g << 8) | b;
}

// Show a raster object with a shift in X and Y
// The shift values are in the range [0, 1) and represent the fraction of the width/height to shift
// this can be used to animate a raster object by moving it across the display in both directions
void show_raster_object_with_shift(int i, float shift_x, float shift_y)
{
    raster_object_t raster = get_raster(i);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in put_raster_object: %i\n", i);
        return;
    }
    int width = raster.width;
    int height = raster.height;
    // Convert shift values to pixel space with 16-bit fixed-point precision
    int dx = (int)(shift_x * width * 65536);
    int dy = (int)(shift_y * height * 65536);

    int shift_x_int = dx >> 16; // Integer pixel shift
    int shift_y_int = dy >> 16;
    int fx = 0xFFFF - dx & 0xFFFF; // Fractional part (16-bit precision)
    int fy = 0xFFFF - dy & 0xFFFF;

    for (int y = 0; y < height; y++)
    {
        // Compute wrapped Y indices using modulo
        int y0 = (y - shift_y_int + height) % height;
        int y1 = (y0 + 1) % height;

        for (int x = 0; x < width; x++)
        {
            // Compute wrapped X indices using modulo
            int x0 = (x - shift_x_int + width) % width;
            int x1 = (x0 + 1) % width;

            // Fetch four neighboring pixels
            uint32_t c00 = raster.raster[y0][x0];

            uint32_t c10 = raster.raster[y0][x1];
            uint32_t c01 = raster.raster[y1][x0];
            uint32_t c11 = raster.raster[y1][x1];
            //  Apply bilinear interpolation using 16-bit integer math
            put_pixel(raster.pixel_mapping[y][x].board, raster.pixel_mapping[y][x].strip, raster.pixel_mapping[y][x].pixel, bilinear_interpolate(c00, c10, c01, c11, fx, fy));
        }
    }
}
