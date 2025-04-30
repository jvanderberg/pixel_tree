#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "defines.h"
#include "types.h"
#include "libpixelblit.h"
#include "utils.h"
#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

uint current_buffer = 0;
// Up to 100 raster objects
raster_object_t *raster_object[MAX_RASTER_OBJECTS];

// double buffer the state of the pixel strip, since we update next version in parallel with DMAing out old version
value_bits_t buffers[2][BOARDS][NUM_PIXELS * 3];
static uint16_t slices[512] = {32768, 33088, 33408, 33728, 34048, 34368, 34688, 35008, 35328, 35648, 35968, 36288, 36608, 36928, 37248, 37568, 37888, 38208, 38528, 38848, 39168, 39488, 39808, 40128, 40448, 40768, 41088, 41408, 41728, 42048, 42368, 42688, 43008, 43271, 43534, 43798, 44061, 44324, 44588, 44851, 45115, 45378, 45641, 45905, 46168, 46431, 46695, 46958, 47222, 47485, 47748, 48012, 48275, 48538, 48802, 49065, 49329, 49592, 49855, 50119, 50382, 50645, 50909, 51172, 51436, 51606, 51778, 51950, 52122, 52293, 52465, 52637, 52809, 52979, 53151, 53323, 53495, 53666, 53838, 54010, 54182, 54352, 54524, 54696, 54868, 55039, 55211, 55383, 55555, 55725, 55897, 56069, 56241, 56412, 56584, 56756, 56928, 56990, 57053, 57115, 57178, 57240, 57303, 57365, 57428, 57490, 57553, 57615, 57678, 57740, 57803, 57865, 57928, 57990, 58053, 58115, 58178, 58240, 58303, 58365, 58428, 58490, 58553, 58615, 58678, 58740, 58803, 58865, 58928, 58865, 58803, 58740, 58678, 58615, 58553, 58490, 58428, 58365, 58303, 58240, 58178, 58115, 58053, 57990, 57928, 57865, 57803, 57740, 57678, 57615, 57553, 57490, 57428, 57365, 57303, 57240, 57178, 57115, 57053, 56990, 56928, 56756, 56584, 56412, 56241, 56069, 55897, 55725, 55555, 55383, 55211, 55039, 54868, 54696, 54524, 54352, 54182, 54010, 53838, 53666, 53495, 53323, 53151, 52979, 52809, 52637, 52465, 52293, 52122, 51950, 51778, 51606, 51436, 51172, 50909, 50645, 50382, 50119, 49855, 49592, 49329, 49065, 48802, 48538, 48275, 48012, 47748, 47485, 47222, 46958, 46695, 46431, 46168, 45905, 45641, 45378, 45115, 44851, 44588, 44324, 44061, 43798, 43534, 43271, 43008, 42688, 42368, 42048, 41728, 41408, 41088, 40768, 40448, 40128, 39808, 39488, 39168, 38848, 38528, 38208, 37888, 37568, 37248, 36928, 36608, 36288, 35968, 35648, 35328, 35008, 34688, 34368, 34048, 33728, 33408, 33088, 32768, 32446, 32124, 31802, 31480, 31158, 30836, 30514, 30192, 29870, 29548, 29226, 28904, 28582, 28260, 27938, 27616, 27294, 26972, 26650, 26328, 26006, 25684, 25362, 25040, 24718, 24396, 24074, 23752, 23430, 23108, 22786, 22464, 22190, 21916, 21642, 21368, 21094, 20820, 20546, 20272, 19998, 19724, 19450, 19176, 18902, 18628, 18354, 18080, 17806, 17532, 17258, 16984, 16710, 16436, 16162, 15888, 15614, 15340, 15066, 14792, 14518, 14244, 13970, 13696, 13496, 13296, 13096, 12897, 12697, 12497, 12297, 12098, 11898, 11698, 11498, 11299, 11099, 10899, 10699, 10500, 10300, 10100, 9900, 9701, 9501, 9301, 9101, 8902, 8702, 8502, 8302, 8103, 7903, 7703, 7503, 7304, 7179, 7054, 6929, 6804, 6679, 6554, 6429, 6304, 6179, 6054, 5929, 5804, 5679, 5554, 5429, 5304, 5179, 5054, 4929, 4804, 4679, 4554, 4429, 4304, 4179, 4054, 3929, 3804, 3679, 3554, 3429, 3304, 3241, 3179, 3116, 3054, 2991, 2929, 2866, 2804, 2741, 2679, 2616, 2554, 2491, 2429, 2366, 2304, 2241, 2179, 2116, 2054, 1991, 1929, 1866, 1804, 1741, 1679, 1616, 1554, 1491, 1429, 1366, 1304, 1275, 1247, 1219, 1191, 1163, 1135, 1107, 1079, 1050, 1022, 994, 966, 938, 910, 882, 854, 825, 797, 769, 741, 713, 685, 657, 629, 600, 572, 544, 516, 488, 460, 432, 404, 393, 382, 372, 361, 350, 340, 329, 319, 308, 297, 287, 276, 265, 255, 244, 234, 223, 212, 202, 191, 180, 170, 159, 149, 138, 127, 117, 106, 95, 85, 74, 64, 62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2};

static int total_area = 1 << 16;
int raster_object_count = -1;

// This is the fractional areas taken by a bin that starts at a given point.
// The bins are broken into 256 slices, and the gaussian spans at most 2 bins, so there
// are 512 slices. The first 256 slices are the first bin, and the second 256 slices are
// the second bin.

// Given that the gausian is centered in a bin, with fractional offset between 0 and 255, this returns the portion of
// the bin to the left (a0), the current bin (a1), and the bin to the right (a2) - (this also applies to up/down).
// The three areas should total 65535.

Bins_t bin_pixel(uint8_t frac_offset)
{
    Bins_t bins = {0, 0, 0};
    uint8_t i = 255 - frac_offset;
    bins.a1 = slices[i];
    bins.a2 = slices[(i + 256)];
    bins.a0 = total_area - bins.a1 - bins.a2;
    // printf("Bin Pixel: %d %d %d %d\n", bins.a0, bins.a1, bins.a2, bins.a0 + bins.a1 + bins.a2);
    return bins;
}
int create_raster(uint16_t height, uint16_t width, uint board, uint strip, uint start_pixel, WrapMode wrap)

{
    printf("Creating raster object\n");
    printf("Height: %d, Width: %d\n", height, width);
    raster_object_count++;
    if (raster_object_count >= MAX_RASTER_OBJECTS)
    {
        printf("Max raster objects reached, not creating new raster object\n");
        return -1;
    }
    printf("Creating raster object %d\n", raster_object_count);
    uint offset = 0;
    uint pixel = 0;

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
            raster->pixel_mapping[i][j].pixel = offset + start_pixel;
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
                    offset = (width * (current_wrap)) - (pixel + 1 - (width * (current_wrap - 1)));
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
            raster->pixel_mapping[i][j].pixel = offset + start_pixel;
            printf("Board: %d, Strip: %d, Pixel: %d\n", board, strip, offset + start_pixel);
            printf("Current wrap: %d\n", current_wrap);
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

    // Leave the first bit zero, as this is not an address line
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
                h = h + (float)i / raster.height / 3;
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

    // printf("Weights: %d \n", w00 + w11 + w01 + w10);

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
void show_raster_object_with_shift2(int i, uint64_t shift_x, uint64_t shift_y)
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
    int dx = (int)(shift_x) % (65536 * width);
    int dy = (int)(shift_y) % (65536 * height);
    printf("Shift: %d %d\n", dx, dy);
    int shift_x_int = dx >> 16; // Integer pixel shift
    int shift_y_int = dy >> 16;
    printf("Shift: %d %d\n", shift_x_int, shift_y_int);
    uint16_t fx = (0xFFFF - dx & 0xFFFF) % 0xFFFF; // Fractional part (16-bit precision)
    uint16_t fy = (0xFFFF - dy & 0xFFFF) % 0xFFFF;

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

static inline uint8_t saturating_add(uint8_t a, uint8_t b)
{
    uint16_t sum = a + b;
    return sum | -(sum >> 8);
}

void show_raster_object_with_shift(int i, uint64_t shift_x, uint64_t shift_y)
{
    raster_object_t raster = get_raster(i);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        printf("Invalid raster object in put_raster_object: %i\n", i);
        return;
    }
    int width = raster.width;
    int height = raster.height;
    uint32_t *buffer = malloc(width * height * sizeof(uint32_t));
    memset(buffer, 0, sizeof(*buffer) * width * height);
    show_raster_object_with_shift_internal(buffer, i, shift_x, shift_y);
    // Store the results
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            put_pixel(raster.pixel_mapping[y][x].board, raster.pixel_mapping[y][x].strip, raster.pixel_mapping[y][x].pixel, buffer[y * width + x]);
        }
    }
    free(buffer);
}

void show_raster_objects_with_shift(int count, int i[], uint64_t shift_x[], uint64_t shift_y[])
{
    // All rasters must be the same size, use the first for the buffer
    raster_object_t raster = get_raster(i[0]);
    if (raster.raster == NULL || raster.pixel_mapping == NULL)
    {
        // printf("Invalid raster object in put_raster_object: %i\n", i);
        return;
    }
    int width = raster.width;
    int height = raster.height;
    uint32_t *buffer = malloc(width * height * sizeof(uint32_t));
    memset(buffer, 0, sizeof(*buffer) * width * height);
    for (int j = 0; j < count; j++)
    {
        show_raster_object_with_shift_internal(buffer, i[j], shift_x[j], shift_y[j]);
    }
    // Store the results
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            put_pixel(raster.pixel_mapping[y][x].board, raster.pixel_mapping[y][x].strip, raster.pixel_mapping[y][x].pixel, buffer[y * width + x]);
        }
    }
    free(buffer);
}
// Show a raster object with a shift in X and Y
// The shift values store the integer offset in the first 16 bits, the fractional part in the last 16 bits
// this can be used to animate a raster object by moving it across the display in both directions
void show_raster_object_with_shift_internal(uint32_t *buffer, int i, uint64_t shift_x, uint64_t shift_y)
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
    int dx = (int)(shift_x) % (65536 * width);
    int dy = (int)(shift_y) % (65536 * height);

    int shift_x_int = dx >> 16; // Integer pixel shift
    int shift_y_int = dy >> 16;

    uint16_t fx = (dx & 0xFFFF) % 0xFFFF; // Fractional part (16-bit precision)
    uint16_t fy = (dy & 0xFFFF) % 0xFFFF;

    // zero out the buffer

    for (int y = 0; y < height; y++)
    {
        // Compute wrapped Y indices using modulo
        int y0 = (y - shift_y_int + height) % height;

        for (int x = 0; x < width; x++)
        {

            // Compute wrapped X indices using modulo
            int x0 = (x - shift_x_int + width) % width;
            uint32_t val = raster.raster[y0][x0];
            // printf("Val: %d %d %d %d %d\n", val, x0, y0, x, y);

            if (!val)
            {
                continue;
            }

            Bins_t h = bin_pixel(fx >> 8);
            Bins_t v = bin_pixel(fy >> 8);
            uint32_t hor[3] = {h.a0, h.a1, h.a2};
            uint32_t ver[3] = {v.a0, v.a1, v.a2};
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    // Get the RGB values from the pixel
                    int r = (val >> 16) & 0xFF;
                    int g = (val >> 8) & 0xFF;
                    int b = val & 0xFF;
                    // Apply the Gaussian weights to the RGB values
                    // You need to multiply the weights from the x and y direction
                    // and then normalize the result
                    uint16_t w = hor[i] * ver[j] >> 16;
                    uint8_t wr = (r * w) >> 16;
                    uint8_t wg = (g * w) >> 16;
                    uint8_t wb = (b * w) >> 16;
                    int x1 = (x + (i - 1) + width) % width;
                    int y1 = (y + (j - 1) + height) % height;
                    uint32_t c = buffer[y1 * width + x1];
                    uint8_t r2 = (c >> 16) & 0xFF;
                    uint8_t g2 = (c >> 8) & 0xFF;
                    uint8_t b2 = c & 0xFF;

                    // Blend existing pixel color with the new color
                    uint8_t rf = saturating_add(r2, wr);
                    uint8_t gf = saturating_add(g2, wg);
                    uint8_t bf = saturating_add(b2, wb);
                    buffer[y1 * width + x1] = rf << 16 | gf << 8 | bf;
                }
            }
        }
    }
}

static uint64_t start_time = 0;

void start_timers()
{
    start_time = time_us_64();
}

uint64_t stop_timers(const char *log_message)
{
    if (start_time == 0)
    {
        printf("Error: Timer was not started.\n");
        return 0;
    }
    uint64_t elapsed_time = (time_us_64() - start_time); // Convert to milliseconds
    printf("%s: Elapsed time = %llu us\n", log_message, elapsed_time);
    start_time = 0; // Reset timer
    return elapsed_time;
}
