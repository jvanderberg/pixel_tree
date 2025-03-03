/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "lib/pixelblit.h"
#include "lib/utils.h"

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

// void print_current_buffer(uint board)
// {
//     printf("Current buffer:\n");

//     for (int i = 0; i < NUM_PIXELS * 3; i++)
//     {
//         printf("Pixel %d: \n", i);
//         for (int j = VALUE_PLANE_COUNT - 1; j >= 0; j--)
//         {
//             for (int k = 0; k < 32; k++)
//             {
//                 printf("%d", (buffers[current_buffer][board][i].planes[j] >> k) & 1);
//             }
//             printf("\n");
//         }
//         printf("\n");
//     }
//     printf("\n");
// }

uint64_t timer_start()
{
    return time_us_64();
}

double timer_end(uint64_t start, const char *comment)
{
    uint64_t end_us = time_us_64();
    // Calculate elapsed time in seconds
    double elapsed = (double)(end_us - start);
    printf("%s: %dµs\n", comment, (int)elapsed);
    return elapsed;
}

uint64_t last_sparkle_time = 0;

// Define a type for the scheduled functions
typedef void (*ScheduledFunction)(void *);

// Struct to hold function details
typedef struct
{
    ScheduledFunction function;
    void *param;
    uint32_t time_seconds;
} FunctionSchedule;

uint64_t start_time = 0;
size_t current_index = 0;
// Function to execute scheduled tasks
// void run_scheduler(FunctionSchedule *schedule, size_t count)
// {

//     float elapsed_time = (time_us_64() - start_time) / 1000000; // Convert to seconds
//     // Check if params are not null
//     if (schedule[current_index].param != NULL)
//     {
//         // Call the function with the params
//         schedule[current_index].function(schedule[current_index].param);
//     }
//     else
//     { // Call the the function without parameters

//         schedule[current_index].function(NULL);
//     }

//     // Check if the time for the current function has expired
//     if (elapsed_time >= schedule[current_index].time_seconds)
//     {

//         // Reset the timer for this function and move to the next
//         start_time = time_us_64();
//         current_index = (current_index + 1) % count; // Loop back to the first function
//     }
// }

int current_strip = 0;

// void diag()
// {
//     float h = 0;
//     float s = 1;
//     float l = 0.5;

//     // Run every 16ms
//     if (time_us_64() - last_rainbow_time < 250000)
//     {
//         return;
//     }
//     else
//     {
//         last_rainbow_time = time_us_64();
//     }

//     current_strip = (current_strip + 1) % STRIPS;
//     for (int i = 0; i < STRIPS; i++)
//     {
//         for (int j = 0; j < NUM_PIXELS; j++)
//         {
//             if (i == current_strip)
//             {
//                 display[0][i][j] = 0x0000ff;
//             }
//             else
//             {
//                 display[0][i][j] = 0x000000;
//             }
//         }
//     }
// }
void set_string(uint board, uint strip, uint32_t color)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        // display[board][strip][i] = color;
    }
}

// void run_sparkle(void *param)
// {
//     uint32_t color = *(uint32_t *)param;
//     // Run every 16ms
//     if (time_us_64() - last_sparkle_time < 16000)
//     {
//         return;
//     }
//     else
//     {
//         last_sparkle_time = time_us_64();
//     }
//     for (uint board = 0; board < BOARDS; board++)
//     {
//         for (int i = 0; i < STRIPS; i++)
//         {
//             for (int j = 0; j < NUM_PIXELS; j++)
//             {
//                 uint32_t rgb = display[board][i][j];
//                 // Randomly every 200th time reset the pixel to white
//                 if (rand() % 100 == 0)
//                 {
//                     display[board][i][j] = color;
//                 }
//                 else
//                 {
//                     display[board][i][j] = fade_rgb(rgb, 245u);
//                 }
//             }
//         }
//     }
// }

uint64_t last_shooting_star_time = 0;

int star_pixels[STRIPS] = {0};
void init_shooting_star(void *param)
{
    for (int i = 0; i < STRIPS; i++)
    {
        star_pixels[i] = rand() % NUM_PIXELS;
    }
}

// void shooting_star(void *param)
// {
//     uint fade_amount = 250;
//     fade(&fade_amount);
//     uint32_t color = *(uint32_t *)param;
//     // Run every 16ms
//     if (time_us_64() - last_shooting_star_time < 32000)
//     {
//         return;
//     }
//     else
//     {
//         last_shooting_star_time = time_us_64();
//     }
//     if (rand() % 100 == 0)
//     {

//         star_pixels[rand() % STRIPS] = rand() % NUM_PIXELS;
//     }

//     for (uint board = 0; board < BOARDS; board++)
//     {
//         for (int i = 0; i < STRIPS; i++)
//         {
//             int pos = star_pixels[i];
//             if (pos > 0)
//             {
//                 display[board][i][pos] = color;
//                 star_pixels[i]--;
//             }
//             else
//             {
//                 star_pixels[i] = NUM_PIXELS - 1;
//             }
//         }
//     }
// }

int spiral_location[2] = {0, 0};
int spiral_location2[2] = {0, 0};

void init_spiral(void *param)
{
    for (uint board = 0; board < BOARDS; board++)
    {
        for (int i = 0; i < STRIPS; i++)
        {
            for (int j = 0; j < NUM_PIXELS; j++)
            {
                // display[board][i][j] = 0x000000;
            }
        }
    }
    spiral_location[0] = 0;
    spiral_location[1] = 0;
    spiral_location2[0] = 12;
    spiral_location2[1] = 1;
}

void spiral(void *param)
{

    uint32_t color = *(uint32_t *)param;
    if (time_us_64() - last_sparkle_time < 1000)
    {
        return;
    }
    else
    {
        last_sparkle_time = time_us_64();
    }
    int fade_amount = 254;
    // fade(&fade_amount);
    for (uint board = 0; board < BOARDS; board++)
    {

        spiral_location[0] = (spiral_location[0] + 1);
        if (spiral_location[0] >= STRIPS)
        {
            spiral_location[0] = 0;
            spiral_location[1] = (spiral_location[1] + 3) % NUM_PIXELS;
        }
        // display[board][spiral_location[0]][spiral_location[1]] = color;
        spiral_location2[0] = (spiral_location2[0] + 1);
        if (spiral_location2[0] >= STRIPS)
        {
            spiral_location2[0] = 0;
            spiral_location2[1] = (spiral_location2[1] + 3) % NUM_PIXELS;
        }
        // display[board][spiral_location2[0]][spiral_location2[1]] = 0x00ff00;
    }
}

uint32_t four_color_palette[6] = {0xff0000, 0x00ff00, 0x0000ff, 0x00ffff, 0xffff00, 0xff00ff};
uint32_t last_fourcolor_time = 0;

void four_color(void *param)
{
    if (time_us_64() - last_fourcolor_time < 2000000)
    {
        return;
    }
    else
    {
        last_fourcolor_time = time_us_64();
    }

    for (uint board = 0; board < BOARDS; board++)
    {
        for (int i = 0; i < STRIPS; i++)
        {
            for (int j = 0; j < NUM_PIXELS; j++)
            {
                // Put 4 colors on the display somewhat randomly
                // display[board][i][j] = four_color_palette[rand() % 6];
            }
        }
    }
}

void solid_color(void *param)
{
    uint32_t color = *(uint32_t *)param;
    for (uint board = 0; board < BOARDS; board++)
    {
        for (int i = 0; i < STRIPS; i++)
        {
            for (int j = 0; j < NUM_PIXELS; j++)
            {
                //  display[board][i][j] = color;
            }
        }
    }
}
uint32_t sparkle_color = 0x0000ff;
uint32_t white = 0xffffff;
uint32_t black = 0x000000;
uint32_t red = 0xff0000;
uint32_t green = 0x00ff00;
uint32_t blue = 0x0000ff;
uint fade_slow = 253;
int fade_millis = 50;
FunctionSchedule schedule[] = {
    {init_spiral, NULL, 1},
    // {spiral, &red, 60},
    // {fade, &fade_slow, 2},
    // {four_color, NULL, 60},
    // {solid_color, &green, 60},     // Call every second
    // {init_shooting_star, NULL, 1}, // Call every second
    // {shooting_star, &white, 60},   // Call every 2 seconds
    // {fade, &fade_slow, 2},         // Call every 3 seconds
    // {run_sparkle, &white, 2}, // Call every 2 seconds
    // {fade, &fade_slow, 2},    // Call every 3 seconds
    // {init_rainbow, NULL, 1},  // Call every second
    // {rainbow, NULL, 60}};     // Call every 3 seconds
    // {fade, &fade_slow, 2},
    // {init_shooting_star, NULL, 1},
    // {shooting_star, &blue, 60},
    // {fade, &fade_slow, 3}, // Call every 2 seconds
    // {solid_color, &red, 60},
    // {run_sparkle, &blue, 60},
    // {spiral, &white, 60},
    // {fade, &fade_slow, 2},
    // {init_shooting_star, NULL, 1},
    // {shooting_star, &green, 60},
    // {solid_color, &blue, 60}};
};

uint64_t my_timer;
int main()
{
    start_time = time_us_64();
    stdio_init_all();
   

    initialize_dma();
    for (int pin = 0; pin <= 3; pin++)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT); // Set as output
    }
    int board1 = create_raster(16, 100, 0, 0, 0, CLIP);
    int board2 = create_raster(16, 100, 9, 0, 0, CLIP);

    init_rainbow(board1);
    init_rainbow(board2);
    int time = 0;
    while (1)
    {
        // rainbow(board1);
        //  fill_raster(board2, 0xff0000);
        //  rainbow(board2);
        // sleep_ms(16);
        float shift_x = fmodf(time * 0.01f, 1.0f); // Move right over time
        float shift_y = fmodf(time * 0.01f, 1.0f);
        show_raster_object_with_shift(board1, shift_x, shift_y);
        show_raster_object_with_shift(board2, shift_x, shift_y);
        show_pixels();
        time++;
    }
    remove_dma();
}

    