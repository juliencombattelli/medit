#ifndef MEDIT_TYPES_H_
#define MEDIT_TYPES_H_

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
        .r = (unsigned char)((code >> (8 * 3)) & 0xFF),
        .g = (unsigned char)((code >> (8 * 2)) & 0xFF),
        .b = (unsigned char)((code >> (8 * 1)) & 0xFF),
        .a = (unsigned char)((code >> (8 * 0)) & 0xFF),
    };
}

static inline Color color_inverse(Color color)
{
    return (Color) {
        .r = 255 - color.r,
        .g = 255 - color.g,
        .b = 255 - color.b,
        .a = 255 - color.a,
    };
}

typedef struct {
    union {
        size_t v[2];
        struct {
            size_t col, row;
        };
    };
} Cell;

#endif // MEDIT_TYPES_H_
