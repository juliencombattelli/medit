#include "sdl3_internal.h"

#include "utils/utils.h"

#include <core/utils.h>

#include <string.h>

// Allocate a null-terminated string copy from the per-frame arena
// Returns NULL if the arena is full (the label is silently dropped)
// TODO reallocate a new arena if full? or use buckets?
// const char* display_sdl3_arena_str(SDL3Display* display, const char* str, size_t len)
// {
//     size_t needed = len + 1;
//     if (display->ui_text_arena_used + needed > TEXT_ARENA_SIZE) {
//         (void)fprintf(stderr, "ui_text_arena full: dropping label\n");
//         return NULL;
//     }
//     char* dst = &display->ui_text_arena[display->ui_text_arena_used];
//     memcpy(dst, str, len);
//     dst[len] = '\0';
//     display->ui_text_arena_used += needed;
//     return dst;
// }

void display_sdl3_clear(SDL3Display* display)
{
    Color color = display->medit->config.theme.color_scheme.editor_bg;
    SDL_SetRenderDrawColor(display->renderer, color_to_RGBA_args(color));
    SDL_RenderClear(display->renderer);
}

void display_sdl3_draw_text(
    SDL3Display* display,
    const char* text,
    size_t len,
    Font* font,
    PixelPos pos,
    Color color)
{
    if (text == NULL || len == 0) {
        return;
    }

    TTF_Text* text_obj = display->text_cache;

    TTF_SetTextFont(text_obj, font->main);
    TTF_SetTextString(text_obj, text, len);
    TTF_SetTextColor(text_obj, color_to_RGBA_args(color));

    TTF_DrawRendererText(text_obj, (float)pos.x, (float)pos.y + (float)font->line_centering_offset);
}

void display_sdl3_render_frame(SDL3Display* display)
{
    SDL_RenderPresent(display->renderer);
}
