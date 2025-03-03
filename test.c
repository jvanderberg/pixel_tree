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

    return 0;
}