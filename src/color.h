#ifndef MEDIT_COLOR_H_
#define MEDIT_COLOR_H_

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
        .r = (code >> (8 * 3)) & 0xFF,
        .g = (code >> (8 * 2)) & 0xFF,
        .b = (code >> (8 * 1)) & 0xFF,
        .a = (code >> (8 * 0)) & 0xFF,
    };
}

#endif // MEDIT_COLOR_H_
