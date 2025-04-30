#ifdef LOCAL_TESTING
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include "lib/utils.h"
#include "lib/defines.h"
#include <assert.h>

void printBinary(const char *description, unsigned int number)
{
    printf("%s: ", description); // Print the description
    for (int i = 31; i >= 0; i--)
    { // Iterate through the bits
        printf("%c", (number & (1 << i)) ? '1' : '0');
        if (i % 4 == 0 && i != 0)
        { // Add a space every 4 bits
            printf(" ");
        }
    }
    printf("\n"); // Newline at the end
}

uint16_t gaussian[] = {291, 1038, 3118, 7907, 16931, 30608, 46723, 60220, 65535, 60220, 46723, 30608, 16931, 7907, 3118, 1038, 291};
// 64,320,896,1984,3904,6336,8768,10304,10304,8768,6336,3904,1984,896,320,64
uint32_t areas[] = {64, 340, 900, 2000, 4000, 6392, 8768, 10304, 10304, 8768, 6392, 4000, 2000, 900, 340, 64};

// uint32_t areas[] = {21264, 66496, 176400, 397408, 760624, 1237296, 1711088, 2012080, 2012080, 1711088, 1237296, 760624, 397408, 176400, 66496, 21264};
// int total_area = 1 << 16;

//
uint32_t gaussian_interpolate(uint32_t offset)
{
    uint32_t index = (offset + 31) >> 5;

    int area = 0;
    for (int i = index; i < index + 7 && i < 16; i++)
    {

        area += areas[i];
        // printf("Area %d %i\n", areas[i], i);
    }

    // Perfectly aligned on a border
    if (offset % 32 == 0)
    {
        if (index + 7 < 16)
        {
            area += areas[index + 7];
        }
        // area += areas[index - 1];

        // printf("Area: %d %i\n", areas[index + 7], index + 7);
        // printf("Area: %d %i\n", areas[index - 1], index - 1);
        // printf("%f %i %i", (float)area / (float)total_area, total_area, area);
        return area;
    }
    else
    {

        // Get the portion of the first bin, take the area of that bin,
        // and multiple by the width of the overlap, which is 32 - the remainder of the offset divided by 32.
        // Then divide by (left shift 5) to get the fractional area
        int frac_area = (areas[index - 1] * (32 - offset % 32)) >> 5;
        // printf("Frac Area: %d %d\n", frac_area, frac);
        // printf("Area: %d %i %i\n", frac_area, index - 1, areas[index - 1]);
        area += frac_area;
        if (index + 7 < 16)
        {

            // Get the portion of the last bin, take the area of that bin,
            // and multiple by the width of the overlap, which is the remainder of the offset divided by 32
            // Then divide by (left shift 5) to get the fractional area
            int frac_area2 = (areas[index + 7] * (offset % 32)) >> 5;
            area += frac_area2;
            // printf("Area: %d %i %i\n", frac_area2, index + 7, areas[index + 6]);
        }
    }

    // printf("%f %i %i", (float)area / (float)total_area, total_area, area);

    return area;
}

// Takes grid offset from the position of the actual pixel and returns
// the weight for the grid location
// uint32_t gaussian_interpolate2(uint32_t offset)
// {

//     int total2 = 0;
//     for (int i = 0; i < 16; i++)
//     {
//         int d = (i + 1) * 32;
//         int a = gaussian[i];
//         int b = gaussian[i + 1];
//         uint32_t area = 0;
//         if (b > a)
//         {
//             area = 32 * a + (32 * (b - a) / 2);
//         }
//         else
//         {
//             area = 32 * b + (32 * (a - b) / 2);
//         }

//         total_area += area;
//         // 12765312
//         //  2012080
//         printf("%d,", 64 * ((area << 10) / 12765312));
//         total2 += 64 * ((area << 10) / 12765312);
//         // printf("Total: %d\n", total);
//     }

//     printf("Total2: %d\n", total2);
//     for (int i = 0; i < 16; i++)
//     {
//         int d = (i + 1) * 32;
//         int a = gaussian[i];
//         int b = gaussian[i + 1];
//         int area = 0;
//         if (b > a)
//         {
//             area = 32 * a + (32 * (b - a) / 2);
//         }
//         else
//         {
//             area = 32 * b + (32 * (a - b) / 2);
//         }
//         area = area >> 1;

//         printf("Frac Area: %i %d\n", area, i * 32);
//     }
//     printf("Total: %d\n", total_area);
//     return 0;
// }

// Show a raster object with a shift in X and Y
// The shift values are in the range [0, 1) and represent the fraction of the width/height to shift
// this can be used to animate a raster object by moving it across the display in both directions
// void show_raster_object_with_shift2(int i, uint64_t shift_x, uint64_t shift_y)
// {
//     raster_object_t raster = get_raster(i);
//     if (raster.raster == NULL || raster.pixel_mapping == NULL)
//     {
//         printf("Invalid raster object in put_raster_object: %i\n", i);
//         return;
//     }
//     int width = raster.width;
//     int height = raster.height;
//     // Convert shift values to pixel space with 16-bit fixed-point precision
//     int dx = (int)(shift_x) % (65536 * width);
//     int dy = (int)(shift_y) % (65536 * height);

//     int shift_x_int = dx >> 16; // Integer pixel shift
//     int shift_y_int = dy >> 16;
//     printf("Shift: %d %d\n", dx, dy);

//     uint16_t fx = (dx & 0xFFFF) % 0xFFFF; // Fractional part (16-bit precision)
//     uint16_t fy = (dy & 0xFFFF) % 0xFFFF;
//     printf("Shift: %d %d\n", fx, fy);
//     // Create a buffer to hold the pixel values
//     uint32_t *buffer = malloc(width * height * sizeof(uint32_t));

//     for (int y = 0; y < height; y++)
//     {
//         // Compute wrapped Y indices using modulo
//         int y0 = (y - shift_y_int + height) % height;

//         for (int x = 0; x < width; x++)
//         {

//             // Compute wrapped X indices using modulo
//             int x0 = (x - shift_x_int + width) % width;
//             uint32_t val = raster.raster[y0][x0];
//             // printf("Val: %d %d %d %d %d\n", val, x0, y0, x, y);

//             if (!val)
//             {
//                 continue;
//             }
//             printf("Val: %d %d %d %d\n", val, x0, y0, x);
//             printf("Shift: %d %d\n", fx, fy);
//             Bins h = bin_pixel(fx >> 8);
//             Bins v = bin_pixel(fy >> 8);
//             uint32_t hor[3] = {h.a0, h.a1, h.a2};
//             uint32_t ver[3] = {v.a0, v.a1, v.a2};
//             for (int i = 0; i < 3; i++)
//             {
//                 for (int j = 0; j < 3; j++)
//                 {
//                     // Get the RGB values from the pixel
//                     int r = (val >> 16) & 0xFF;
//                     int g = (val >> 8) & 0xFF;
//                     int b = val & 0xFF;
//                     // Apply the Gaussian weights to the RGB values
//                     uint16_t w = hor[i] * ver[j] >> 16;
//                     // printf("Hor: %d %d %d %d\n", hor[i], ver[j], w, hor[i] * ver[j]);
//                     uint8_t wr = (r * w) >> 16;
//                     uint8_t wg = (g * w) >> 16;
//                     uint8_t wb = (b * w) >> 16;
//                     int x1 = (x + (i - 1) + width) % width;
//                     int y1 = (y + (j - 1) + height) % height;
//                     printf("Hor: %d %d %d %d %d\n", x1, y1, wb, w, val);
//                     uint32_t c = buffer[y1 * width + x1];
//                     uint8_t r2 = (c >> 16) & 0xFF;
//                     uint8_t g2 = (c >> 8) & 0xFF;
//                     uint8_t b2 = c & 0xFF;

//                     // Blend existing pixel color with the new color
//                     uint8_t rf = (uint8_t)(((uint16_t)r2 + (uint16_t)wr) % 256);
//                     uint8_t gf = (uint8_t)(((uint16_t)g2 + (uint16_t)wg) % 256);
//                     uint8_t bf = (uint8_t)(((uint16_t)b2 + (uint16_t)wb) % 256);
//                     // printf("Buffer: %d %d %d %d %d %d %d %d\n", rf, gf, bf, r2, g2, b2, wb, w);
//                     buffer[y1 * width + x1] = rf << 16 | gf << 8 | bf;
//                 }
//             }
//         }
//     }
//     // Diplay the buffer
//     for (int y = 0; y < height; y++)
//     {
//         for (int x = 0; x < width; x++)
//         {

//             uint32_t val = buffer[y * width + x];
//             // Display the pixel value
//             printf("%d\t", val);
//         }
//         printf("\n");
//     }
// }

int main()
{
    printBinary("Test", 0x12345678);
    uint obj = create_raster(16, 10, 0, 3, 0, CLIP);
    printf("Object: %d\n", obj);
    raster_object_t ro = get_raster(obj);
    printf("Height: %d\n", ro.height);
    printf("Width: %d\n", ro.width);
    assert(ro.height == 16);
    assert(ro.width == 10);
    assert(ro.raster != NULL);
    assert(ro.pixel_mapping != NULL);
    assert(ro.pixel_mapping[0][0].board == 0);
    assert(ro.pixel_mapping[0][0].strip == 3);
    assert(ro.pixel_mapping[0][0].pixel == 0);
    assert(ro.pixel_mapping[0][1].board == 0);
    assert(ro.pixel_mapping[0][1].strip == 3);
    assert(ro.pixel_mapping[0][1].pixel == 1);
    assert(ro.pixel_mapping[0][10].board == 0);
    assert(ro.pixel_mapping[0][10].strip == 4);

    assert(ro.pixel_mapping[0][10].pixel == 0);
    assert(ro.pixel_mapping[15][9].board == 1);

    assert(ro.pixel_mapping[15][9].strip == 2);

    assert(ro.pixel_mapping[15][9].pixel == 9);

    // Wrapped, 25 x 3
    uint obj2 = create_raster(3, 25, 0, 0, 0, WRAP);
    printf("Object: %d\n", obj);
    raster_object_t ro2 = get_raster(obj2);

    assert(ro2.height == 3);
    assert(ro2.width == 25);
    assert(ro2.raster != NULL);
    assert(ro2.pixel_mapping != NULL);
    assert(ro2.pixel_mapping[0][0].board == 0);
    assert(ro2.pixel_mapping[0][0].strip == 0);
    assert(ro2.pixel_mapping[0][0].pixel == 0);
    assert(ro2.pixel_mapping[0][1].board == 0);
    assert(ro2.pixel_mapping[0][1].strip == 0);
    assert(ro2.pixel_mapping[0][1].pixel == 1);
    assert(ro2.pixel_mapping[1][0].board == 0);
    assert(ro2.pixel_mapping[1][0].strip == 0);

    assert(ro2.pixel_mapping[1][0].pixel == 49);
    assert(ro2.pixel_mapping[2][24].pixel == 74);

    uint obj3 = create_raster(6, 25, 0, 0, 0, NO_WRAP);
    printf("Object 3: %d\n", obj);
    raster_object_t ro3 = get_raster(obj3);

    assert(ro3.height == 6);
    assert(ro3.width == 25);
    assert(ro3.raster != NULL);
    assert(ro3.pixel_mapping != NULL);
    assert(ro3.pixel_mapping[0][0].board == 0);
    assert(ro3.pixel_mapping[0][0].strip == 0);
    assert(ro3.pixel_mapping[0][0].pixel == 0);
    assert(ro3.pixel_mapping[0][1].board == 0);
    assert(ro3.pixel_mapping[0][1].strip == 0);
    assert(ro3.pixel_mapping[0][1].pixel == 1);
    assert(ro3.pixel_mapping[1][0].board == 0);
    assert(ro3.pixel_mapping[1][0].strip == 0);
    assert(ro3.pixel_mapping[1][0].pixel == 25);
    assert(ro3.pixel_mapping[2][24].pixel == 74);
    assert(ro3.pixel_mapping[5][24].pixel == 74);
    assert(ro3.pixel_mapping[5][24].strip == 1);

    // width does not evenly divide NUM_PIXELS, WRAP mode disabled, NO_WRAP defaulting
    // Get same results as above
    uint obj4 = create_raster(6, 52, 0, 0, 0, WRAP);
    printf("Object 4: %d\n", obj);
    raster_object_t ro4 = get_raster(obj3);

    assert(ro4.height == 6);
    assert(ro4.width == 25);
    assert(ro4.raster != NULL);
    assert(ro4.pixel_mapping != NULL);
    assert(ro3.pixel_mapping[0][0].board == 0);
    assert(ro3.pixel_mapping[0][0].strip == 0);
    assert(ro3.pixel_mapping[0][0].pixel == 0);
    assert(ro3.pixel_mapping[0][1].board == 0);
    assert(ro3.pixel_mapping[0][1].strip == 0);
    assert(ro3.pixel_mapping[0][1].pixel == 1);
    assert(ro3.pixel_mapping[1][0].board == 0);
    assert(ro3.pixel_mapping[1][0].strip == 0);
    assert(ro3.pixel_mapping[1][0].pixel == 25);
    assert(ro3.pixel_mapping[2][24].pixel == 74);
    assert(ro3.pixel_mapping[5][24].pixel == 74);
    assert(ro3.pixel_mapping[5][24].strip == 1);

    uint obj5 = create_raster(20, 75, 0, 0, 0, NO_WRAP);
    printf("Object 5: %d\n", obj);
    raster_object_t ro5 = get_raster(obj5);
    assert(ro5.height == 20);
    assert(ro5.width == 75);
    assert(ro5.raster != NULL);
    assert(ro5.pixel_mapping != NULL);
    assert(ro5.pixel_mapping[15][74].board == 0);
    assert(ro5.pixel_mapping[15][74].strip == 15);
    assert(ro5.pixel_mapping[15][74].pixel == 74);
    assert(ro5.pixel_mapping[19][74].board == 1);
    assert(ro5.pixel_mapping[19][74].strip == 3);
    assert(ro5.pixel_mapping[19][74].pixel == 74);

    uint obj6 = create_raster(12, 75, 1, 4, 0, NO_WRAP);
    printf("Object 6: %d\n", obj);
    raster_object_t ro6 = get_raster(obj6);
    assert(ro6.height == 12);
    assert(ro6.width == 75);
    assert(ro6.raster != NULL);
    assert(ro6.pixel_mapping != NULL);
    assert(ro6.pixel_mapping[0][0].board == 1);
    assert(ro6.pixel_mapping[0][0].strip == 4);
    assert(ro6.pixel_mapping[0][0].pixel == 0);
    assert(ro6.pixel_mapping[11][74].board == 1);
    assert(ro6.pixel_mapping[11][74].strip == 15);
    assert(ro6.pixel_mapping[11][74].pixel == 74);

    fill_raster(obj6, 0x00ff00);
    fill_raster(obj5, 0xff0000);
    assert(ro6.raster[0][0] == 0x00ff00);
    assert(ro5.raster[0][0] == 0xff0000);
    assert(ro6.raster[11][74] == 0x00ff00);
    assert(ro5.raster[19][74] == 0xff0000);

    draw_pixel(obj6, 0, 0, 0x0000ff);
    draw_pixel(obj5, 0, 0, 0x0000ff);
    assert(ro6.raster[0][0] == 0x0000ff);
    assert(ro5.raster[0][0] == 0x0000ff);
    assert(ro6.raster[11][74] == 0x00ff00);
    assert(ro5.raster[19][74] == 0xff0000);
    fade_raster(obj6, 255);
    fade_raster(obj5, 255);

    assert(ro6.raster[0][0] == 0x0000fe);
    assert(ro5.raster[0][0] == 0x0000fe);
    assert(ro6.raster[11][74] == 0x00fe00);
    assert(ro5.raster[19][74] == 0xfe0000);
    fade_raster(obj6, 0);
    fade_raster(obj5, 0);
    assert(ro6.raster[0][0] == 0x0);
    assert(ro5.raster[0][0] == 0x0);
    assert(ro6.raster[11][74] == 0x0);
    assert(ro5.raster[19][74] == 0x0);
    uint obj7 = create_raster(16, 75, 0, 0, 0, NO_WRAP);
    draw_pixel(obj7, 0, 0, 0x0000ff);
    show_raster_object(obj7);
    printf("Current buffer: %d\n", current_buffer);
    printBinary("Buffer zerp", buffers[current_buffer][0][2].planes[0]);
    fill_raster(obj7, 0x0000ff);
    show_raster_object(obj7);
    printBinary("Buffer zerp", buffers[current_buffer][0][2].planes[0]);

    for (int i = 0; i < 256; i++)
    {
        Bins_t b = bin_pixel(i);
        printf("Bin: %d %d %d %d\n", b.a0, b.a1, b.a2, b.a0 + b.a1 + b.a2);
        assert(abs(((int)b.a0 + b.a1 + b.a2) - 65536) == 0);
    }
}