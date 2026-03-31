#ifndef MEDIT_COLOR_H_
#define MEDIT_COLOR_H_

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

static inline Color color_from_u32(uint32_t code)
{
    return (Color) {
        .r = (unsigned char)((code >> (8u * 3u)) & UINT8_MAX),
        .g = (unsigned char)((code >> (8u * 2u)) & UINT8_MAX),
        .b = (unsigned char)((code >> (8u * 1u)) & UINT8_MAX),
        .a = (unsigned char)((code >> (8u * 0u)) & UINT8_MAX),
    };
}

static inline Color color_inverse(Color color)
{
    return (Color) {
        .r = 255 - color.r,
        .g = 255 - color.g,
        .b = 255 - color.b,
        .a = color.a,
    };
}

#endif // MEDIT_COLOR_H_
