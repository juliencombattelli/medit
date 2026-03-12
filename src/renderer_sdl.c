#include "renderer_sdl.h"

#include <stdio.h>

#define FORCE_MONOSPACE_FONTS

#define sdl_assert(condition, sdl_call)                                        \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(                                                           \
                stderr,                                                        \
                "%s:%d: Error: %s: %s\n",                                      \
                __FILE__,                                                      \
                __LINE__,                                                      \
                (sdl_call),                                                    \
                SDL_GetError());                                               \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static SDL_Color to_sdl_color(Color color)
{
    return (SDL_Color) {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
    };
}

// **Note about emoji**
// A fallback font is used to render emoji. As we want emoji to fit in a grid
// cell, we compute the size of an emoji glyph (currently 😀) so that it is
// equal to the size of a letter glyph in the main editor font. The computation
// is using strings of a hundred glyphs for a better precision.
// This method works well for the currently used fonts and size but it is
// expected to be britle. Also, rendered emoji are quite small and might not be
// properly readable at all...

#define FONT_TEST_CHAR_COUNT (100)

static const char editor_font_test_string[] = //
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM";
_Static_assert(
    sizeof(editor_font_test_string) - 1
        == FONT_TEST_CHAR_COUNT * (sizeof("M") - 1),
    "");

static const char emoji_font_test_string[] = //
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀";
_Static_assert(
    sizeof(emoji_font_test_string) - 1
        == FONT_TEST_CHAR_COUNT * (sizeof("😀") - 1),
    "");

static int glyph_width(TTF_Font* font, const char* s)
{
    int w = 0;
    if (!TTF_MeasureString(font, s, 0, 0, &w, NULL)) {
        printf("Error: failed to get font metric: %s\n", SDL_GetError());
        return 0;
    }
    return w / FONT_TEST_CHAR_COUNT;
}

static TTF_Font* load_emoji_font_aligned_to_main_font(
    TTF_Font* main_font,
    const char* path,
    int size)
{
#ifdef FORCE_MONOSPACE_FONTS
    const int main_font_w = glyph_width(main_font, editor_font_test_string);
    float factor = 1.0f;
    int iter = 1024;
    while (--iter) {
        TTF_Font* emoji_font = TTF_OpenFont(path, (float)size * factor);
        int emoji_font_w = glyph_width(emoji_font, emoji_font_test_string);
        if (emoji_font_w > main_font_w) {
            factor -= factor * 0.5f;
        } else if (emoji_font_w < main_font_w) {
            factor += factor * 0.5f;
        } else if (emoji_font_w == main_font_w) {
            return emoji_font;
        }
        TTF_CloseFont(emoji_font);
    }
    return NULL;
#else
    return TTF_OpenFont(path, (float)size);
#endif
}

void sdl_render_load_font(
    RendererSDL* renderer,
    Meditor* medit,
    const char* path,
    int size)
{
    (void)medit;

    printf("Info: loading font %s with size %d\n", path, size);
    renderer->font_editor = TTF_OpenFont(path, (float)size);
    if (!renderer->font_editor) {
        printf("Error: failed to load font %s with size %d\n", path, size);
        exit(1);
    }

    TTF_GetStringSize(
        renderer->font_editor,
        "M",
        0,
        &renderer->cell_width,
        &renderer->cell_height);

    TTF_Font* emoji_fallback = load_emoji_font_aligned_to_main_font(
        renderer->font_editor,
        // "asset/font/NotoColorEmoji-Regular.ttf",
        "asset/font/OpenMoji-color-colr0_svg.ttf",
        size);
    if (!emoji_fallback) {
        printf(
            "Warning: failed to find a size aligned to the grid for emoji "
            "font\n");
    } else {
        if (!TTF_AddFallbackFont(renderer->font_editor, emoji_fallback)) {
            printf(
                "Warning: failed to load fallback emoji font: %s\n",
                SDL_GetError());
            exit(1);
        }
    }
}

void sdl_render_unload_font(RendererSDL* renderer, Meditor* medit)
{
    (void)medit;
    TTF_ClearFallbackFonts(renderer->font_editor);
    TTF_CloseFont(renderer->font_editor);
}

int sdl_get_text_cells(RendererSDL* renderer, const char* text)
{
    int text_width = 0;
    TTF_MeasureString(renderer->font_editor, text, 0, 0, &text_width, NULL);
    return text_width / renderer->cell_width;
}

void sdl_render_text0(
    RendererSDL* renderer,
    Meditor* medit,
    const char* text,
    int cell_x,
    int cell_y,
    Color color)
{
    (void)medit;

    size_t text_bytes = strlen(text);

    int text_width = 0;
    size_t text_bytes_max = 0;
    TTF_MeasureString(
        renderer->font_editor,
        text,
        0,
        // take one extra cell to allow display partial chars
        renderer->window_width + renderer->cell_width,
        &text_width,
        &text_bytes_max);

    size_t printed_bytes = SDL_min(text_bytes, text_bytes_max);

    if (text_width == 0) {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(
        renderer->font_editor,
        text,
        printed_bytes,
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
