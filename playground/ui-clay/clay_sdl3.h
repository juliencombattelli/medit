#ifndef CLAY_SDL3_H_
#define CLAY_SDL3_H_

#include "clay.h"

#include "clay_utils.h"

#include "SDL3/SDL_render.h"

//------------------------------------------------------------------------------
// SDL/Clay conversion functions
//------------------------------------------------------------------------------

static inline SDL_FColor clay_to_sdl_color(Clay_Color color)
{
    // Clay internally represents color channels as floats between 0-255
    return (SDL_FColor) {
        .r = color.r / UINT8_MAX,
        .g = color.g / UINT8_MAX,
        .b = color.b / UINT8_MAX,
        .a = color.a / UINT8_MAX,
    };
}

//------------------------------------------------------------------------------
// Mouse handling implementation for SDL
//------------------------------------------------------------------------------

// Ensure clay_utils.h mouse buttons bits are aligned with SDL
// This way no special conversion is needed, SDL_GetMouseState returns a compatible bitset
_Static_assert(SDL_BUTTON_LMASK  == MOUSE_BUTTON_MASK(MOUSE_BUTTON_LEFT), "");
_Static_assert(SDL_BUTTON_MMASK  == MOUSE_BUTTON_MASK(MOUSE_BUTTON_MIDDLE), "");
_Static_assert(SDL_BUTTON_RMASK  == MOUSE_BUTTON_MASK(MOUSE_BUTTON_RIGHT), "");
_Static_assert(SDL_BUTTON_X1MASK == MOUSE_BUTTON_MASK(MOUSE_BUTTON_SIDE_1), "");
_Static_assert(SDL_BUTTON_X2MASK == MOUSE_BUTTON_MASK(MOUSE_BUTTON_SIDE_2), "");

static inline uint32_t sdl_get_mouse_state(MouseState* mouse_state)
{
    return SDL_GetMouseState(&mouse_state->pos.x, &mouse_state->pos.y);
}

//------------------------------------------------------------------------------
// Clay rendering implementation for SDL
//------------------------------------------------------------------------------

void SDL_Clay_RenderClayCommands(SDL_Renderer* renderer, Clay_RenderCommandArray draw_commands);

#endif // CLAY_SDL3_H_
