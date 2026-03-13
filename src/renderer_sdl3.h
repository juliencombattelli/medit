#ifndef MEDIT_RENDERER_SDL3_H_
#define MEDIT_RENDERER_SDL3_H_

#include "color.h"
#include "meditor.h"
#include "renderer.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_editor;
    TTF_Font* font_emoji;
    int cell_width;
    int cell_height;
    int window_width;
    int window_height;
} RendererSDL3;

Renderer create_sdl3_renderer(RendererSDL3* sdl);

#endif // MEDIT_RENDERER_SDL3_H_
