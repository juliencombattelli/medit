#ifndef MEDIT_DISPLAY_SDL3_UTILS_UTILS_H_
#define MEDIT_DISPLAY_SDL3_UTILS_UTILS_H_

#include <core/ui/color.h>
#include <core/ui/rect.h>
#include <core/safeint.h>

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

static inline SDL_Rect rect_to_sdl_rect(Rect r)
{
    return (SDL_Rect) {
        .x = r.x,
        .y = r.y,
        .w = r.w,
        .h = r.h,
    };
}

static inline SDL_FRect rect_to_sdl_frect(Rect r)
{
    return (SDL_FRect) {
        .x = (float)r.x,
        .y = (float)r.y,
        .w = (float)r.w,
        .h = (float)r.h,
    };
}

#endif // MEDIT_DISPLAY_SDL3_UTILS_UTILS_H_
