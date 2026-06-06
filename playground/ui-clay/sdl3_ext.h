#ifndef SDL3_EXT_H_
#define SDL3_EXT_H_

#include "SDL3/SDL_render.h"

void SDL_Ext_RenderFillRoundedRect(
    SDL_Renderer* renderer,
    SDL_FRect rect,
    float corner_radius,
    SDL_FColor color);

#endif // SDL3_EXT_H_
