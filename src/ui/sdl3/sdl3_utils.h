#ifndef MEDIT_UI_SDL3_SDL3_UTILS_H_
#define MEDIT_UI_SDL3_SDL3_UTILS_H_

#include "color.h"
#include "limits.h"

#include <SDL3/SDL.h>

#define try(expr)                                                                                  \
    if (!(expr)) {                                                                                 \
        return false;                                                                              \
    }

#define color_to_RGBA_args(color) (color).r, (color).g, (color).b, (color).a

static inline SDL_Color color_to_sdl3(Color color)
{
    return (SDL_Color) {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
    };
}

static inline int digits_count(int n)
{
    // clang-format off
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    // clang-format on
    return 10;
}

// Maximum number of digits needed to represent any 64-bits integer (including a potential sign for
// signed integers and a null character)
enum {
    INT64_DIGITS_COUNT = 22u,
};

#endif // MEDIT_UI_SDL3_SDL3_UTILS_H_
