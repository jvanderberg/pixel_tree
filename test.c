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

uint16_t slices[512] = {32768, 33088, 33408, 33728, 34048, 34368, 34688, 35008, 35328, 35648, 35968, 36288, 36608, 36928, 37248, 37568, 37888, 38208, 38528, 38848, 39168, 39488, 39808, 40128, 40448, 40768, 41088, 41408, 41728, 42048, 42368, 42688, 43008, 43271, 43534, 43798, 44061, 44324, 44588, 44851, 45115, 45378, 45641, 45905, 46168, 46431, 46695, 46958, 47222, 47485, 47748, 48012, 48275, 48538, 48802, 49065, 49329, 49592, 49855, 50119, 50382, 50645, 50909, 51172, 51436, 51606, 51778, 51950, 52122, 52293, 52465, 52637, 52809, 52979, 53151, 53323, 53495, 53666, 53838, 54010, 54182, 54352, 54524, 54696, 54868, 55039, 55211, 55383, 55555, 55725, 55897, 56069, 56241, 56412, 56584, 56756, 56928, 56990, 57053, 57115, 57178, 57240, 57303, 57365, 57428, 57490, 57553, 57615, 57678, 57740, 57803, 57865, 57928, 57990, 58053, 58115, 58178, 58240, 58303, 58365, 58428, 58490, 58553, 58615, 58678, 58740, 58803, 58865, 58928, 58865, 58803, 58740, 58678, 58615, 58553, 58490, 58428, 58365, 58303, 58240, 58178, 58115, 58053, 57990, 57928, 57865, 57803, 57740, 57678, 57615, 57553, 57490, 57428, 57365, 57303, 57240, 57178, 57115, 57053, 56990, 56928, 56756, 56584, 56412, 56241, 56069, 55897, 55725, 55555, 55383, 55211, 55039, 54868, 54696, 54524, 54352, 54182, 54010, 53838, 53666, 53495, 53323, 53151, 52979, 52809, 52637, 52465, 52293, 52122, 51950, 51778, 51606, 51436, 51172, 50909, 50645, 50382, 50119, 49855, 49592, 49329, 49065, 48802, 48538, 48275, 48012, 47748, 47485, 47222, 46958, 46695, 46431, 46168, 45905, 45641, 45378, 45115, 44851, 44588, 44324, 44061, 43798, 43534, 43271, 43008, 42688, 42368, 42048, 41728, 41408, 41088, 40768, 40448, 40128, 39808, 39488, 39168, 38848, 38528, 38208, 37888, 37568, 37248, 36928, 36608, 36288, 35968, 35648, 35328, 35008, 34688, 34368, 34048, 33728, 33408, 33088, 32768, 32446, 32124, 31802, 31480, 31158, 30836, 30514, 30192, 29870, 29548, 29226, 28904, 28582, 28260, 27938, 27616, 27294, 26972, 26650, 26328, 26006, 25684, 25362, 25040, 24718, 24396, 24074, 23752, 23430, 23108, 22786, 22464, 22190, 21916, 21642, 21368, 21094, 20820, 20546, 20272, 19998, 19724, 19450, 19176, 18902, 18628, 18354, 18080, 17806, 17532, 17258, 16984, 16710, 16436, 16162, 15888, 15614, 15340, 15066, 14792, 14518, 14244, 13970, 13696, 13496, 13296, 13096, 12897, 12697, 12497, 12297, 12098, 11898, 11698, 11498, 11299, 11099, 10899, 10699, 10500, 10300, 10100, 9900, 9701, 9501, 9301, 9101, 8902, 8702, 8502, 8302, 8103, 7903, 7703, 7503, 7304, 7179, 7054, 6929, 6804, 6679, 6554, 6429, 6304, 6179, 6054, 5929, 5804, 5679, 5554, 5429, 5304, 5179, 5054, 4929, 4804, 4679, 4554, 4429, 4304, 4179, 4054, 3929, 3804, 3679, 3554, 3429, 3304, 3241, 3179, 3116, 3054, 2991, 2929, 2866, 2804, 2741, 2679, 2616, 2554, 2491, 2429, 2366, 2304, 2241, 2179, 2116, 2054, 1991, 1929, 1866, 1804, 1741, 1679, 1616, 1554, 1491, 1429, 1366, 1304, 1275, 1247, 1219, 1191, 1163, 1135, 1107, 1079, 1050, 1022, 994, 966, 938, 910, 882, 854, 825, 797, 769, 741, 713, 685, 657, 629, 600, 572, 544, 516, 488, 460, 432, 404, 393, 382, 372, 361, 350, 340, 329, 319, 308, 297, 287, 276, 265, 255, 244, 234, 223, 212, 202, 191, 180, 170, 159, 149, 138, 127, 117, 106, 95, 85, 74, 64, 62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2};

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
int total_area = 1 << 16;

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
typedef struct
{
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;

} Bins;

Bins bin_pixel(uint8_t frac_offset)
{
    Bins bins = {0, 0, 0};
    uint8_t i = 255 - frac_offset;
    bins.a1 = slices[i];
    bins.a2 = slices[i + 256];
    bins.a0 = total_area - bins.a1 - bins.a2;
    printf("Bin Pixel: %d %d %d %d\n", bins.a0, bins.a1, bins.a2, bins.a0 + bins.a1 + bins.a2);
    return bins;
}

// Takes grid offset from the position of the actual pixel and returns
// the weight for the grid location
uint32_t gaussian_interpolate2(uint32_t offset)
{

    int total2 = 0;
    for (int i = 0; i < 16; i++)
    {
        int d = (i + 1) * 32;
        int a = gaussian[i];
        int b = gaussian[i + 1];
        uint32_t area = 0;
        if (b > a)
        {
            area = 32 * a + (32 * (b - a) / 2);
        }
        else
        {
            area = 32 * b + (32 * (a - b) / 2);
        }

        total_area += area;
        // 12765312
        //  2012080
        printf("%d,", 64 * ((area << 10) / 12765312));
        total2 += 64 * ((area << 10) / 12765312);
        // printf("Total: %d\n", total);
    }

    printf("Total2: %d\n", total2);
    for (int i = 0; i < 16; i++)
    {
        int d = (i + 1) * 32;
        int a = gaussian[i];
        int b = gaussian[i + 1];
        int area = 0;
        if (b > a)
        {
            area = 32 * a + (32 * (b - a) / 2);
        }
        else
        {
            area = 32 * b + (32 * (a - b) / 2);
        }
        area = area >> 1;

        printf("Frac Area: %i %d\n", area, i * 32);
    }
    printf("Total: %d\n", total_area);
    return 0;
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

    int shift_x_int = dx >> 16; // Integer pixel shift
    int shift_y_int = dy >> 16;
    printf("Shift: %d %d\n", dx, dy);

    uint16_t fx = (dx & 0xFFFF) % 0xFFFF; // Fractional part (16-bit precision)
    uint16_t fy = (dy & 0xFFFF) % 0xFFFF;
    printf("Shift: %d %d\n", fx, fy);
    // Create a buffer to hold the pixel values
    uint32_t *buffer = malloc(width * height * sizeof(uint32_t));

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
            printf("Val: %d %d %d %d\n", val, x0, y0, x);
            printf("Shift: %d %d\n", fx, fy);
            Bins h = bin_pixel(fx >> 8);
            Bins v = bin_pixel(fy >> 8);
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
                    uint16_t w = hor[i] * ver[j] >> 16;
                    // printf("Hor: %d %d %d %d\n", hor[i], ver[j], w, hor[i] * ver[j]);
                    uint8_t wr = (r * w) >> 16;
                    uint8_t wg = (g * w) >> 16;
                    uint8_t wb = (b * w) >> 16;
                    int x1 = (x + (i - 1) + width) % width;
                    int y1 = (y + (j - 1) + height) % height;
                    printf("Hor: %d %d %d %d %d\n", x1, y1, wb, w, val);
                    uint32_t c = buffer[y1 * width + x1];
                    uint8_t r2 = (c >> 16) & 0xFF;
                    uint8_t g2 = (c >> 8) & 0xFF;
                    uint8_t b2 = c & 0xFF;

                    // Blend existing pixel color with the new color
                    uint8_t rf = (uint8_t)(((uint16_t)r2 + (uint16_t)wr) % 256);
                    uint8_t gf = (uint8_t)(((uint16_t)g2 + (uint16_t)wg) % 256);
                    uint8_t bf = (uint8_t)(((uint16_t)b2 + (uint16_t)wb) % 256);
                    // printf("Buffer: %d %d %d %d %d %d %d %d\n", rf, gf, bf, r2, g2, b2, wb, w);
                    buffer[y1 * width + x1] = rf << 16 | gf << 8 | bf;
                }
            }
        }
    }
    // Diplay the buffer
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint32_t val = buffer[y * width + x];
            // Display the pixel value
            printf("%d\t", val);
        }
        printf("\n");
    }
}

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

    // assert(buffers[current_buffer][0][0].planes[0] == 0x0000ff);
    // gaussian_interpolate2(15);
    Bins h = bin_pixel(128);
    Bins v = bin_pixel(128);
    uint32_t hor[3] = {h.a0, h.a1, h.a2};
    uint32_t ver[3] = {v.a0, v.a1, v.a2};
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            printf("%d\t", hor[j] * ver[i] / 65535);
        }
        printf("\n");
    }
    uint objanim = create_raster(10, 10, 0, 0, 0, CLIP);
    printf("Object anim: %d\n", objanim);
    raster_object_t roanim = get_raster(objanim);
    assert(roanim.height == 10);
    assert(roanim.width == 10);
    roanim.raster[0][0] = 230;
    roanim.raster[2][2] = 100;
    show_raster_object_with_shift2(objanim, 3 << 16 | 1 << 15, 4 << 16 | 1 << 15);
    // for (int i = 0; i < 512; i++)
    // {
    //     printf("%d,", gaussian_interpolate(i));
    // }
}