# Pi Pico High Speed WS2811 Pixel driver for PixelBlit board

The PixelBlit is a Pi Pico levelshifter/breakout that can drive up to 16 individual WS2811 strings off of one board. You can daisy chain up to 16 boards with using 40 pin IDC connectors (aka IDE cables).

Boards have unique addresses which are software addressable, and the user can create virtual 2D rasters which span multiple boards. Rasters can also address just a subsection of a board. In this way a single Pi Pico (CPU cycles willing) can coordinate a full LED installation, our drive a large, and very bright, pixel display.

Board addresses are set with DIP switches, and board can share the same address, for simple duplication.

## Getting started

Install the Pi Pico SDK
Raspberry Pi Pico Visual Studio Code extension 'raspberry-pi.raspberry-pi-pico'

Clone this repo
mkdir build
cd build
cmake -G Ninja -DLOCAL_BUILD=ON ..
ninja

## Building

cd build
ninja

## Deploy

picotool load pio_ws2812_parallel.elf

## Compile/Run Tests

mkdir test_build
cd test_build
cmake -G Ninja -DLOCAL_BUILD=ON ..
ninja
./test

## PixelBlit programming model

defines.h contains the number of boards, strips, and pixels per strip. Edit this to reference your design. Not all the boards need to have the same number of STRIPS or NUM_PIXELS per strip, but NUM_PIXELS should be the largest number of pixels per strip.

### Creating a raster object

To address the PixelBlit strings, you first create a raster, which is a display buffer with a mapping to physical pixels.

`int create_raster(uint16_t height, uint16_t width, uint board, uint strip, uint pixel, WrapMode wrap);  `

You give the function a height and width. 'height' is the strip direction, width is the pixel direction. You give create_raster a start board, strip, and pixel, and it moves along the board mapping physical pixel addresses into your raster.

WrapMode tells it how to do this:

-   NOWRAP This just progresses linearly filling the raster, moving out along a strip until
    there are no more pixels (NUM_PIXELS is reached), then moving to the next strip, until there are no more strips, and then to next board
-   CLIP This works similarly to NOWRAP, but moves to the next strip as soon as 'width' pixels are found. This is useful if some strips don't actually contain NUM_PIXELS.
-   WRAP This fills pixels, but assumes strings are wrapped in zig zag fashion. For example you could use a 'width' of 25, but have pixel strings which are 100 long, but zig zag 4 times to create 4 rows of 25. WRAP mode will correctly map these pixels, reversing the order of every other column.

## Writing to a raster object

You can call get_raster to get the raster object and write pixels to it

`raster_object_t get_raster(uint raster_id);`

object.raster[x][y] = color

Where color is a 32 bit integer.

This only writes colors to an internal raster buffer. This buffer is persistent, and pixel colors will only change when re-written.

## Writing to the physical strings

`void show_raster_object(int i);`

show_raster_object(raster_id);

Writes the raster to the PIO buffers. PIO buffers rotate the data in a way such that it can be streamed in parallel to the 16 GPIO pins. The PIO buffers are also persistent, and double buffered.

`void show_pixels();`

Will write the PIO buffers to the devices using an async DMA request.

# Custom code

Edit ws2812_parallel.c, or create you own executable by editing CMakeLists.txt
