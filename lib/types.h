#ifndef TYPES_H
#define TYPES_H

#include "defines.h"

#ifdef LOCAL_BUILD
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

typedef struct
{
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
} Bins_t;

typedef enum
{
    CLIP = 0,
    NO_WRAP = 1,
    WRAP = 2,
} WrapMode;

#define VALUE_PLANE_COUNT (8)
typedef struct
{
    // stored MSB first
    uint32_t planes[VALUE_PLANE_COUNT];
} value_bits_t;

typedef struct
{
    uint8_t board;
    uint8_t strip;
    uint8_t pixel;
} pixel_address_t;

typedef struct
{
    uint16_t height;
    uint16_t width;
    uint32_t **raster;
    pixel_address_t **pixel_mapping;
} raster_object_t;

#endif // TYPES_H