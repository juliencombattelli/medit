#include "renderer_sdl.h"

#include <stdio.h>

static void sdl_assert(bool condition, const char* sdl_call)
{
    if (!condition) {
        fprintf(stderr, "Error: %s: %s\n", sdl_call, SDL_GetError());
        exit(1);
    }
}

static SDL_Color to_sdl_color(Color color)
{
    return (SDL_Color) {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
    };
}

void sdl_render_load_font(
    RendererSDL* renderer,
    Meditor* medit,
    const char* path,
    int size)
{
    renderer->font_editor = TTF_OpenFont(path, (float)size);
    TTF_GetStringSize(
        renderer->font_editor,
        "M",
        0,
        &renderer->cell_width,
        &renderer->cell_height);
}

void sdl_render_text0(
    RendererSDL* renderer,
    Meditor* medit,
    const char* text,
    int cell_x,
    int cell_y,
    Color color)
{
    // TODO avoid creating repeatedly the surface and texture

    SDL_Surface* surface = TTF_RenderText_Blended(
        renderer->font_editor,
        text,
        0,
        text ? to_sdl_color(color)
             : (SDL_Color) { .r = 255, .g = 255, .b = 255, .a = 255 });
    sdl_assert(surface, "TTF_RenderText_Blended");

    SDL_Texture* texture = SDL_CreateTextureFromSurface(
        renderer->renderer,
        surface);
    sdl_assert(texture, "SDL_CreateTextureFromSurface");

    const SDL_FRect glyph_rect = {
        .x = (float)(cell_x * renderer->cell_width),
        .y = (float)(cell_y * renderer->cell_height),
        .w = (float)(surface->w),
        .h = (float)(surface->h),
    };

    SDL_RenderTexture(renderer->renderer, texture, NULL, &glyph_rect);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

void sdl_render_cursor(RendererSDL* renderer, Meditor* medit, Color color)
{
    const SDL_FRect cursor_rect = {
        .x = (float)(medit->cursor_col * renderer->cell_width),
        .y = (float)(medit->cursor_row * renderer->cell_height),
        .w = (float)(renderer->cell_width),
        .h = (float)(renderer->cell_height),
    };

    SDL_SetRenderDrawColor(
        renderer->renderer,
        color.r,
        color.g,
        color.b,
        color.a);
    SDL_RenderFillRect(renderer->renderer, &cursor_rect);
}

void sdl_render_debug_grid(RendererSDL* renderer, Meditor* medit)
{
    if (!medit->draw_debug_grid) {
        return;
    }

    int win_width = 0;
    int win_height = 0;
    SDL_GetWindowSize(renderer->window, &win_width, &win_height);

    int grid_rows = win_height / renderer->cell_height;
    int grid_cols = win_width / renderer->cell_width;

    SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 255, 100);

    for (int i = 0; i < grid_cols + 1; i++) {
        const SDL_FRect vertical_line = {
            .x = (float)(i * renderer->cell_width),
            .y = (float)0,
            .w = (float)1,
            .h = (float)win_height,
        };
        SDL_RenderRect(renderer->renderer, &vertical_line);
    }
    for (int i = 0; i < grid_rows + 1; i++) {
        const SDL_FRect horizontal_line = {
            .x = (float)0,
            .y = (float)(i * renderer->cell_height),
            .w = (float)win_width,
            .h = (float)1,
        };
        SDL_RenderRect(renderer->renderer, &horizontal_line);
    }
}
