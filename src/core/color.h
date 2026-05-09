#ifndef MEDIT_CORE_COLOR_H_
#define MEDIT_CORE_COLOR_H_

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

#define color_from_u32(code)                                                                       \
    (Color)                                                                                        \
    {                                                                                              \
        .r = (uint8_t)(((code) >> (8u * 3u)) & UINT8_MAX),                                         \
        .g = (uint8_t)(((code) >> (8u * 2u)) & UINT8_MAX),                                         \
        .b = (uint8_t)(((code) >> (8u * 1u)) & UINT8_MAX),                                         \
        .a = (uint8_t)(((code) >> (8u * 0u)) & UINT8_MAX),                                         \
    }

static inline Color color_inverse(Color color)
{
    return (Color) {
        .r = UINT8_MAX - color.r,
        .g = UINT8_MAX - color.g,
        .b = UINT8_MAX - color.b,
        .a = color.a,
    };
}

#endif // MEDIT_CORE_COLOR_H_
