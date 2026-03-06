#ifndef MEDIT_RENDERER_SDL_H_
#define MEDIT_RENDERER_SDL_H_

#include "color.h"
#include "meditor.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_editor;
    int cell_width;
    int cell_height;
    int window_width;
    int window_height;
} RendererSDL;

void sdl_render_load_font(
    RendererSDL* renderer,
    Meditor* medit,
    const char* path,
    int size);

int sdl_get_text_cells(RendererSDL* renderer, const char* text);

void sdl_render_text0(
    RendererSDL* renderer,
    Meditor* medit,
    const char* text,
    int cell_x,
    int cell_y,
    Color color);

void sdl_render_cursor(RendererSDL* renderer, Meditor* medit, Color color);

void sdl_render_debug_grid(RendererSDL* renderer, Meditor* medit);

#endif // MEDIT_RENDERER_SDL_H_
