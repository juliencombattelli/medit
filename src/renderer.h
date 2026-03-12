#ifndef MEDIT_RENDERER_H_
#define MEDIT_RENDERER_H_

#include "color.h"
#include "meditor.h"
#include "renderer_sdl.h"

#include <stdio.h>

typedef enum {
    RENDERER_SDL,
} RendererId;

typedef struct {
    RendererId id;
    union {
        RendererSDL sdl;
    };
} Renderer;

static inline void render_text0(
    Renderer* renderer,
    Meditor* medit,
    const char* text,
    int cell_x,
    int cell_y,
    Color color)
{
    switch (renderer->id) {
        case RENDERER_SDL:
            sdl_render_text0(&renderer->sdl, medit, text, cell_x, cell_y, color);
            return;
    }
    printf("WARNING: Unimplemented: %s\n", __func__);
}

static inline void render_cursor(Renderer* renderer, Meditor* medit, Color color)
{
    switch (renderer->id) {
        case RENDERER_SDL: sdl_render_cursor(&renderer->sdl, medit, color); return;
    }
    printf("WARNING: Unimplemented: %s\n", __func__);
}

static inline void render_debug_grid(Renderer* renderer, Meditor* medit)
{
    switch (renderer->id) {
        case RENDERER_SDL: sdl_render_debug_grid(&renderer->sdl, medit); return;
    }
    printf("WARNING: Unimplemented: %s\n", __func__);
}

#endif // MEDIT_RENDERER_H_
