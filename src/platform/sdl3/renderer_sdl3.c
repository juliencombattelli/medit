#include "assert.h"
#include "meditor.h"
#include "sdl3.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <limits.h>
#include <stdio.h>

#define DEFAULT_WINDOW_WIDTH           1280
#define DEFAULT_WINDOW_HEIGHT          720
#define DEFAULT_CURSOR_BLINK_PERIOD_MS 1000

typedef struct {
    size_t width;
    size_t height;
} PixelSize;

typedef struct {
    Uint64 last_ticks;
    bool show_cursor;
} CursorBlink;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_editor;
    TTF_Font* font_emoji;
    PixelSize cell_size;
    PixelSize window_size;
    CursorBlink cursor_blink;
    size_t line_nr_padding;
} RendererSDL3;

#define FORCE_MONOSPACE_FONTS

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
// expected to be brittle. Also, rendered emoji are quite small and might not be
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
_Static_assert(sizeof(editor_font_test_string) - 1 == FONT_TEST_CHAR_COUNT * (sizeof("M") - 1), "");

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
_Static_assert(sizeof(emoji_font_test_string) - 1 == FONT_TEST_CHAR_COUNT * (sizeof("😀") - 1), "");

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
#define FORCE_MONOSPACE_FONTS_MAX_ITER 1024
    int iter = FORCE_MONOSPACE_FONTS_MAX_ITER;
    while (--iter) {
        TTF_Font* emoji_font = TTF_OpenFont(path, (float)size * factor);
        int emoji_font_w = glyph_width(emoji_font, emoji_font_test_string);
        if (emoji_font_w > main_font_w) {
            factor -= factor / 2.0f;
        } else if (emoji_font_w < main_font_w) {
            factor += factor / 2.0f;
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

static void resize_window_with_data(Meditor* medit, int w, int h)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    assert(w >= 0);
    assert(h >= 0);

    renderer->window_size.width = (size_t)w;
    renderer->window_size.height = (size_t)h;
}

static void resize_window(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(renderer->window, &w, &h);

    resize_window_with_data(medit, w, h);
}

static void resize_cell(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    int w = 0;
    int h = 0;
    TTF_GetStringSize(renderer->font_editor, "M", 0, &w, &h);
    assert(w >= 0);
    assert(h >= 0);

    renderer->cell_size.width = (size_t)w;
    renderer->cell_size.height = (size_t)h;
}

static void resize_grid(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;
    medit->grid_size = (Cell) {
        .col = renderer->window_size.width / renderer->cell_size.width,
        .row = renderer->window_size.height / renderer->cell_size.height,
    };
}

static void sdl3_renderer_create(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* sdl_window = SDL_CreateWindow(
        "Medit",
        DEFAULT_WINDOW_WIDTH,
        DEFAULT_WINDOW_HEIGHT,
        SDL_WINDOW_HIDDEN);

    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
    SDL_SetRenderVSync(sdl_renderer, 1);

    renderer->window = sdl_window;
    renderer->renderer = sdl_renderer;

    SDL_ShowWindow(renderer->window);

    SDL_StartTextInput(renderer->window);

    meditor_load_default_gui_keybind(medit);
}

static void sdl3_render_load_font(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    printf(
        "Info: loading font %s with size %d\n",
        medit->config.editor_font_path,
        medit->config.editor_font_size);
    renderer->font_editor = TTF_OpenFont(
        medit->config.editor_font_path,
        (float)medit->config.editor_font_size);
    if (!renderer->font_editor) {
        printf(
            "Error: failed to load font %s with size %d\n",
            medit->config.editor_font_path,
            medit->config.editor_font_size);
        abort();
    }

    resize_cell(medit);
    resize_window(medit);
    resize_grid(medit);

    renderer->font_emoji = load_emoji_font_aligned_to_main_font(
        renderer->font_editor,
        // "asset/font/NotoColorEmoji-Regular.ttf",
        "asset/font/OpenMoji-color-colr0_svg.ttf",
        medit->config.editor_font_size);
    if (!renderer->font_emoji) {
        printf(
            "Warning: failed to find a size aligned to the grid for emoji "
            "font\n");
    } else {
        if (!TTF_AddFallbackFont(renderer->font_editor, renderer->font_emoji)) {
            printf("Warning: failed to load fallback emoji font: %s\n", SDL_GetError());
            abort();
        }
    }
}

static void sdl3_render_unload_font(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    TTF_ClearFallbackFonts(renderer->font_editor);
    TTF_CloseFont(renderer->font_emoji);
    TTF_CloseFont(renderer->font_editor);
}

static size_t sdl3_get_text_cells(Meditor* medit, const char* text)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    int text_width = 0;
    TTF_MeasureString(renderer->font_editor, text, 0, 0, &text_width, NULL);
    assert(text_width >= 0);
    return (size_t)text_width / renderer->cell_size.width;
}

static void sdl3_clear_screen(Meditor* medit, Color color)
{
    SDL_Renderer* renderer = ((RendererSDL3*)medit->renderer.data)->renderer;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer);
}

static void update_cursor_blinking_timer(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    Uint64 ticks = SDL_GetTicks();
    if (ticks - renderer->cursor_blink.last_ticks > DEFAULT_CURSOR_BLINK_PERIOD_MS / 2) {
        renderer->cursor_blink.show_cursor = !renderer->cursor_blink.show_cursor;
        medit->input_in_frame = true; // force to redraw screen
        renderer->cursor_blink.last_ticks = ticks;
    }
}

static void reset_cursor_blinking_time(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;
    renderer->cursor_blink.last_ticks = SDL_GetTicks();
    renderer->cursor_blink.show_cursor = true;
}

static void sdl3_handle_events(Meditor* medit)
{
    update_cursor_blinking_timer(medit);

    SDL_Event event = { 0 };
    while (SDL_PollEvent(&event) != 0) {
        medit->input_in_frame = true;
        reset_cursor_blinking_time(medit);
        switch (event.type) {
            case SDL_EVENT_QUIT: medit->running = false; break;
            case SDL_EVENT_WINDOW_RESIZED:
                resize_window_with_data(medit, (int)event.window.data1, (int)event.window.data2);
                resize_grid(medit);
                break;
            case SDL_EVENT_KEY_DOWN: {
                KeybindEvent keybind_event = keybind_sdl3_translate_event(&event);
                if (keybind_handle_event(&medit->keybind, &keybind_event)) {
                    break;
                }
                switch (event.key.key) {
                    case SDLK_RETURN: meditor_split_line(medit); break;
                    case SDLK_BACKSPACE: meditor_erase_char(medit); break;
                    default: break;
                }
            } break;
            case SDL_EVENT_TEXT_INPUT: {
                const size_t text_cells = medit_get_text_cells(medit, event.text.text);
                size_t text_len = strlen(event.text.text);
                meditor_insert_text(medit, event.text.text, text_len, text_cells);
                meditor_cursor_right(medit, text_cells);
            } break;
            case SDL_EVENT_KEYMAP_CHANGED: {
                printf("Reloading keymapping\n");
                keybind_reinit(&medit->keybind);
                meditor_load_default_gui_keybind(medit);
            } break;
            default: break;
        }
    }
}

static int count_digits(int n)
{
    if (n < 0) {
        n = (n == INT_MIN) ? INT_MAX : -n;
    }
    if (n < 10) {
        return 1;
    }
    if (n < 100) {
        return 2;
    }
    if (n < 1000) {
        return 3;
    }
    if (n < 10000) {
        return 4;
    }
    if (n < 100000) {
        return 5;
    }
    if (n < 1000000) {
        return 6;
    }
    if (n < 10000000) {
        return 7;
    }
    if (n < 100000000) {
        return 8;
    }
    if (n < 1000000000) {
        return 9;
    }
    /*      2147483647 is 2^31-1 - add more ifs as needed
    and adjust this final return as well. */
    return 10;
}

static void render_line_nr(Meditor* medit, Cell cell, Color color)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    // Compute the maximum number of digits (minimum 4)
    const size_t line_count = SDL_max(medit->focused_view.file->lines.count, 1000);
    const int max_digits = count_digits((int)line_count);

    // Compute the padding size to render this max line number
    renderer->line_nr_padding = 10;
    char line_count_str[1024] = { 0 };
    (void)sprintf(line_count_str, "%zu", line_count);
    int line_number_width = 0;
    TTF_MeasureString(renderer->font_editor, line_count_str, 0, 0, &line_number_width, NULL);
    assert(line_number_width >= 0);
    renderer->line_nr_padding += (size_t)line_number_width;

    // Render the number of the current line
    char current_line_nr_str[1024] = { 0 };
    (void)sprintf(current_line_nr_str, "%*zu", max_digits, cell.row + 1);

    SDL_Surface* surface = TTF_RenderText_Blended(
        renderer->font_editor,
        current_line_nr_str,
        0,
        to_sdl_color(color));
    assert(surface);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer->renderer, surface);
    assert(texture != NULL);

    const SDL_FRect glyph_rect = {
        .x = 0,
        .y = (float)(cell.row * renderer->cell_size.height),
        .w = (float)(surface->w),
        .h = (float)(surface->h),
    };

    SDL_RenderTexture(renderer->renderer, texture, NULL, &glyph_rect);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

static void sdl3_render_text(Meditor* medit, const char* text, size_t n, Cell cell, Color color)
{
    render_line_nr(medit, cell, color);

    if (text == NULL || n == 0) {
        return;
    }

    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    // take one extra cell to allow display partial chars
    size_t max_string_width = renderer->window_size.width + renderer->cell_size.width;
    assert(max_string_width <= INT_MAX);

    int text_width = 0;
    size_t text_bytes_max = 0;
    TTF_MeasureString(
        renderer->font_editor,
        text,
        n,
        (int)max_string_width,
        &text_width,
        &text_bytes_max);

    size_t printed_bytes = SDL_min((size_t)n, text_bytes_max);

    if (text_width == 0) {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(
        renderer->font_editor,
        text,
        printed_bytes,
        text ? to_sdl_color(color) : (SDL_Color) { .r = 255, .g = 255, .b = 255, .a = 255 });
    assert(surface);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer->renderer, surface);
    assert(texture != NULL);

    const SDL_FRect glyph_rect = {
        .x = (float)(renderer->line_nr_padding + (cell.col * renderer->cell_size.width)),
        .y = (float)(cell.row * renderer->cell_size.height),
        .w = (float)(surface->w),
        .h = (float)(surface->h),
    };

    SDL_RenderTexture(renderer->renderer, texture, NULL, &glyph_rect);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

static void sdl3_render_text0(Meditor* medit, const char* text, Cell cell, Color color)
{
    sdl3_render_text(medit, text, strlen(text), cell, color);
}

static void sdl3_render_cursor(Meditor* medit, Color color)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    if (!renderer->cursor_blink.show_cursor) {
        return;
    }

    Color inverse_color = {
        .r = 255 - color.r,
        .g = 255 - color.g,
        .b = 255 - color.b,
        .a = 255 - color.a,
    };

    for (size_t i = 0; i < medit->focused_view.cursors.count; ++i) {
        Cell* cursor = &medit->focused_view.cursors.items[i];

        const SDL_FRect cursor_rect = {
            .x = (float)(renderer->line_nr_padding + (cursor->col * renderer->cell_size.width)),
            .y = (float)(cursor->row * renderer->cell_size.height),
            .w = (float)(renderer->cell_size.width),
            .h = (float)(renderer->cell_size.height),
        };

        SDL_SetRenderDrawColor(renderer->renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer->renderer, &cursor_rect);

        Line* current_line = &medit->focused_view.file->lines.items[cursor->row];
        if (cursor->col < current_line->count) {
            const char* c = &current_line->items[cursor->col];
            sdl3_render_text(medit, c, 1, *cursor, inverse_color);
        }
    }
}

static void sdl3_render_debug_grid(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    if (!medit->draw_debug_grid) {
        return;
    }

    SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 255, 255);

    for (size_t i = 0; i < medit->grid_size.col + 1; i++) {
        const SDL_FRect vertical_line = {
            .x = (float)(i * renderer->cell_size.width),
            .y = (float)0,
            .w = (float)1,
            .h = (float)renderer->window_size.height,
        };
        SDL_RenderRect(renderer->renderer, &vertical_line);
    }
    for (size_t i = 0; i < medit->grid_size.row + 1; i++) {
        const SDL_FRect horizontal_line = {
            .x = (float)0,
            .y = (float)(i * renderer->cell_size.height),
            .w = (float)renderer->window_size.width,
            .h = (float)1,
        };
        SDL_RenderRect(renderer->renderer, &horizontal_line);
    }
}

static void sdl3_present(Meditor* medit)
{
    SDL_Renderer* renderer = ((RendererSDL3*)medit->renderer.data)->renderer;

    SDL_RenderPresent(renderer);
}

static void sdl3_destroy(Meditor* medit)
{
    RendererSDL3* renderer = (RendererSDL3*)medit->renderer.data;

    SDL_StopTextInput(renderer->window);

    SDL_DestroyRenderer(renderer->renderer);
    SDL_DestroyWindow(renderer->window);

    TTF_Quit();
    SDL_Quit();

    free(renderer);
}

Renderer renderer_sdl3(void)
{
    return (Renderer) {
        .data = calloc(1, sizeof(RendererSDL3)),
        .fns = {
            .create = sdl3_renderer_create,
            .load_font = sdl3_render_load_font,
            .unload_font = sdl3_render_unload_font,
            .get_text_cells = sdl3_get_text_cells,
            .handle_events = sdl3_handle_events,
            .clear_screen = sdl3_clear_screen,
            .render_text = sdl3_render_text,
            .render_text0 = sdl3_render_text0,
            .render_cursor = sdl3_render_cursor,
            .render_debug_grid = sdl3_render_debug_grid,
            .present = sdl3_present,
            .destroy = sdl3_destroy,
        },
        .name = "SDL3",
    };
}
