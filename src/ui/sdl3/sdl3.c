#include "sdl3.h"
#include "assert.h"
#include "dynarray.h"
#include "font.h"
#include "keybind.h"
#include "meditor.h"
#include "safeint.h"
#include "unicode.h"
#include "utils.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <limits.h>

typedef struct {
    int width;
    int height;
} PixelSize;

typedef struct {
    int x;
    int y;
} PixelPos;

typedef struct {
    Uint64 last_ticks;
    bool show_cursor;
} CursorBlink;

typedef struct {
    SDL_PropertiesID props;
    TTF_Font* main;
    TTF_Font* emoji;
} Font;

typedef struct {
    TTF_Text* text; // cached line content TTF_Text, NULL if not yet created
    const char* line_ptr; // line->items at cache time, for invalidation
    size_t line_count; // line->count at cache time, for invalidation
    TTF_Text* nr_text; // cached line number TTF_Text, NULL if not yet created
    int cached_nr; // row+1 at cache time, for invalidation
    int cached_nr_digits; // line_nr_max_digits at cache time, for invalidation
} CachedLine;

typedef struct {
    CachedLine* items;
    size_t count;
    size_t capacity;
    File* file; // file this cache belongs to, NULL if invalid
} LineCache;

typedef struct {
    Meditor* medit;
    SDL_Window* window;
    SDL_Renderer* renderer;
    Font font_ui;
    Font font_editor;
    PixelSize cell_size;
    PixelSize window_size;
    CursorBlink cursor_blink;
    int line_nr_padding;
    int line_nr_max_digits;
    int editor_font_size;
    TTF_TextEngine* text_engine;
    LineCache* group_caches;
    size_t group_caches_count;
} SDL3Ui;

static bool ui_sdl3_create(SDL3Ui* ui, Meditor* medit);
static void ui_sdl3_destroy(SDL3Ui* ui);

static void ui_sdl3_resize_window_with_data(SDL3Ui* ui, PixelSize window_size);
static void ui_sdl3_resize_window(SDL3Ui* ui);
static void ui_sdl3_resize_cell(SDL3Ui* ui);

static void ui_sdl3_load_ui_font(SDL3Ui* ui);
static void ui_sdl3_unload_ui_font(SDL3Ui* ui);
static void ui_sdl3_load_editor_font(SDL3Ui* ui);
static void ui_sdl3_unload_editor_font(SDL3Ui* ui);

static void ui_sdl3_handle_event(SDL3Ui* ui);

static void ui_sdl3_clear(SDL3Ui* ui);
static void ui_sdl3_draw_text(
    SDL3Ui* ui,
    const char* text,
    size_t len,
    Font* font,
    PixelPos pos,
    Color color);
static void ui_sdl3_draw_cursor(SDL3Ui* ui, FileViewGroup* group);
static void ui_sdl3_render(SDL3Ui* ui);

static void temp_ui_sdl3_update_file_view_groups_size(SDL3Ui* ui);
static void ui_sdl3_invalidate_line_caches(SDL3Ui* ui);

enum {
    DEFAULT_WINDOW_WIDTH = 1280,
    DEFAULT_WINDOW_HEIGHT = 720,
    DEFAULT_CURSOR_BLINK_PERIOD_MS = 1000,
};

static void ui_sdl3_invalidate_line_caches(SDL3Ui* ui)
{
    for (size_t g = 0; g < ui->group_caches_count; ++g) {
        LineCache* cache = &ui->group_caches[g];
        for (size_t i = 0; i < cache->count; ++i) {
            if (cache->items[i].text) {
                TTF_DestroyText(cache->items[i].text);
                cache->items[i].text = NULL;
            }
            if (cache->items[i].nr_text) {
                TTF_DestroyText(cache->items[i].nr_text);
                cache->items[i].nr_text = NULL;
            }
        }
        cache->count = 0;
        cache->file = NULL;
    }
}

static void ui_sdl3_ensure_group_caches(SDL3Ui* ui, size_t group_idx)
{
    if (group_idx >= ui->group_caches_count) {
        size_t new_count = group_idx + 1;
        ui->group_caches = realloc(ui->group_caches, new_count * sizeof(LineCache));
        assert(ui->group_caches != NULL && "Buy more RAM lol");
        memset(
            ui->group_caches + ui->group_caches_count,
            0,
            (new_count - ui->group_caches_count) * sizeof(LineCache));
        ui->group_caches_count = new_count;
    }
}

static void ui_sdl3_sync_group_cache(SDL3Ui* ui, size_t group_idx, FileView* file_view)
{
    LineCache* cache = &ui->group_caches[group_idx];
    File* file = file_view->file;

    if (cache->file != file) {
        for (size_t i = 0; i < cache->count; ++i) {
            if (cache->items[i].text) {
                TTF_DestroyText(cache->items[i].text);
                cache->items[i].text = NULL;
            }
            if (cache->items[i].nr_text) {
                TTF_DestroyText(cache->items[i].nr_text);
                cache->items[i].nr_text = NULL;
            }
        }
        cache->count = 0;
        cache->file = file;
    }

    size_t new_count = file->lines.count;

    // Destroy TTF_Text objects for lines that were removed
    for (size_t i = new_count; i < cache->count; ++i) {
        if (cache->items[i].text) {
            TTF_DestroyText(cache->items[i].text);
            cache->items[i].text = NULL;
        }
        if (cache->items[i].nr_text) {
            TTF_DestroyText(cache->items[i].nr_text);
            cache->items[i].nr_text = NULL;
        }
    }

    // Grow cache array and zero out new slots
    if (new_count > cache->count) {
        dynarray_reserve(cache, new_count);
        memset(cache->items + cache->count, 0, (new_count - cache->count) * sizeof(CachedLine));
    }
    cache->count = new_count;
}

static bool ui_sdl3_create(SDL3Ui* ui, Meditor* medit)
{
    try(SDL_Init(SDL_INIT_VIDEO));
    try(TTF_Init());

    SDL_Window* window = SDL_CreateWindow(
        "Medit",
        DEFAULT_WINDOW_WIDTH,
        DEFAULT_WINDOW_HEIGHT,
        SDL_WINDOW_HIDDEN);
    try(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    try(renderer);

    printf("[DEBUG] Selected renderer: %s\n", SDL_GetRendererName(renderer));

    try(SDL_SetRenderVSync(renderer, 1));

    TTF_TextEngine* text_engine = TTF_CreateRendererTextEngine(renderer);
    try(text_engine);

    ui->window = window;
    ui->renderer = renderer;
    ui->text_engine = text_engine;

    try(SDL_ShowWindow(ui->window));

    try(SDL_StartTextInput(ui->window));

    medit_load_default_gui_keybind(medit);

    ui->medit = medit;

    return true;
}

static void ui_sdl3_destroy(SDL3Ui* ui)
{
    SDL_StopTextInput(ui->window);

    ui_sdl3_invalidate_line_caches(ui);
    for (size_t g = 0; g < ui->group_caches_count; ++g) {
        free(ui->group_caches[g].items);
    }
    free(ui->group_caches);
    TTF_DestroyRendererTextEngine(ui->text_engine);

    SDL_DestroyRenderer(ui->renderer);
    SDL_DestroyWindow(ui->window);

    TTF_Quit();
    SDL_Quit();

    *ui = (SDL3Ui) { 0 };
}

static void ui_sdl3_resize_window_with_data(SDL3Ui* ui, PixelSize window_size)
{
    assert(window_size.width >= 0);
    assert(window_size.height >= 0);

    ui->window_size = window_size;
}

static void ui_sdl3_resize_window(SDL3Ui* ui)
{
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(ui->window, &w, &h);

    ui_sdl3_resize_window_with_data(ui, (PixelSize) { .width = w, .height = h });
}

static void ui_sdl3_resize_cell(SDL3Ui* ui)
{
    int w = 0;
    int h = 0;
    TTF_GetStringSize(ui->font_editor.main, "M", 0, &w, &h);
    assert(w >= 0);
    assert(h >= 0);

    ui->cell_size = (PixelSize) { .width = w, .height = h };
}

// static void ui_sdl3_load_ui_font(SDL3Ui* ui);
// static void ui_sdl3_unload_ui_font(SDL3Ui* ui);

static void ui_sdl3_load_editor_font(SDL3Ui* ui)
{
    Meditor* medit = ui->medit;

    if (ui->font_editor.props == 0) {
        ui->font_editor.props = SDL_CreateProperties();
        assert(ui->font_editor.props != 0);
    }

    SDL_SetStringProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_FILENAME_STRING,
        medit->config.editor_font_path);
    SDL_SetFloatProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_SIZE_FLOAT,
        (float)medit->config.editor_font_size);
    SDL_SetNumberProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_HORIZONTAL_DPI_NUMBER,
        FONT_DPI_DEFAULT);
    SDL_SetNumberProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_VERTICAL_DPI_NUMBER,
        FONT_DPI_DEFAULT);

    ui->font_editor.main = TTF_OpenFontWithProperties(ui->font_editor.props);
    if (!ui->font_editor.main) {
        printf(
            "Error: failed to load font %s with size %d\n",
            medit->config.editor_font_path,
            medit->config.editor_font_size);
        abort();
    }

    ui_sdl3_resize_cell(ui);
    ui_sdl3_resize_window(ui);

    ui->font_editor.emoji = load_emoji_font_aligned_to(
        ui->font_editor.main,
        "asset/font/NotoColorEmoji-Regular.ttf",
        // "asset/font/OpenMoji-color-colr0_svg.ttf",
        // "asset/font/seguiemj.ttf",
        medit->config.editor_font_size,
        0); // Do not align the emoji font width to the main font width
    if (!ui->font_editor.emoji) {
        printf(
            "Warning: failed to find a size aligned to the grid for emoji "
            "font\n");
    } else {
        if (!TTF_AddFallbackFont(ui->font_editor.main, ui->font_editor.emoji)) {
            printf("Warning: failed to load fallback emoji font: %s\n", SDL_GetError());
            abort();
        }
    }
}

static void ui_sdl3_unload_editor_font(SDL3Ui* ui)
{
    ui_sdl3_invalidate_line_caches(ui);
    TTF_ClearFallbackFonts(ui->font_editor.main);
    TTF_CloseFont(ui->font_editor.main);
    TTF_CloseFont(ui->font_editor.emoji);
    ui->font_editor = (Font) { 0 };
}

static void ui_sdl3_on_window_resized(SDL3Ui* ui, int w, int h)
{
    ui_sdl3_resize_window_with_data(
        ui,
        (PixelSize) {
            .width = w,
            .height = h,
        });

    temp_ui_sdl3_update_file_view_groups_size(ui);
}

static void ui_sdl3_update_cursor_blinking_timer(SDL3Ui* ui)
{
    Uint64 ticks = SDL_GetTicks();
    if (ticks - ui->cursor_blink.last_ticks > DEFAULT_CURSOR_BLINK_PERIOD_MS / 2) {
        ui->cursor_blink.show_cursor = !ui->cursor_blink.show_cursor;
        ui->medit->input_in_frame = true; // Force to redraw screen
        ui->cursor_blink.last_ticks = ticks;
    }
}

static void ui_sdl3_reset_cursor_blinking_timer(SDL3Ui* ui)
{
    ui->cursor_blink.last_ticks = SDL_GetTicks();
    ui->cursor_blink.show_cursor = true;
}

static void ui_sdl3_on_text_input(SDL3Ui* ui, const char* text)
{
    size_t text_len = strlen(text);
    medit_insert_text(ui->medit, text, text_len);
    medit_cursor_right(ui->medit);
}

static void ui_sdl3_on_key_down(SDL3Ui* ui, SDL_Event* event)
{
    switch (event->key.key) {
        case SDLK_RETURN: medit_split_line(ui->medit); break;
        case SDLK_BACKSPACE: medit_erase_char(ui->medit); break;
        default: break;
    }
}

static void ui_sdl3_handle_event(SDL3Ui* ui)
{
    Meditor* medit = ui->medit;

    ui_sdl3_update_cursor_blinking_timer(ui);

    // Save current font size to monitor changes
    ui->editor_font_size = medit->config.editor_font_size;

    SDL_Event event = { 0 };
    while (SDL_PollEvent(&event) != 0) {
        medit->input_in_frame = true;
        // Force cursor to reappear at each keystroke (for any event actually) by reseting its timer
        ui_sdl3_reset_cursor_blinking_timer(ui);
        switch (event.type) {
            case SDL_EVENT_QUIT: medit->running = false; break;
            case SDL_EVENT_WINDOW_RESIZED:
                ui_sdl3_on_window_resized(ui, event.window.data1, event.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN: {
                KeybindEvent keybind_event = keybind_sdl3_translate_event(&event);
                if (keybind_handle_event(&medit->keybind, &keybind_event)) {
                    break;
                }
                ui_sdl3_on_key_down(ui, &event);
            } break;
            case SDL_EVENT_TEXT_INPUT: {
                ui_sdl3_on_text_input(ui, event.text.text);
            } break;
            case SDL_EVENT_KEYMAP_CHANGED: {
                printf("Reloading keymapping\n");
                keybind_reinit(&medit->keybind);
                medit_load_default_gui_keybind(medit);
            } break;
            default: break;
        }
    }
}

static void ui_sdl3_clear(SDL3Ui* ui)
{
    Color color = ui->medit->config.color_theme.editor_bg;

    SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(color));
    SDL_RenderClear(ui->renderer);
}

static void ui_sdl3_draw_text(
    SDL3Ui* ui,
    const char* text,
    size_t len,
    Font* font,
    PixelPos pos,
    Color color)
{
    if (text == NULL || len == 0) {
        return;
    }

    TTF_Text* t = TTF_CreateText(ui->text_engine, font->main, text, len);
    assert(t != NULL);
    assert(TTF_SetTextColor(t, color_to_RGBA_args(color)));
    TTF_DrawRendererText(t, (float)pos.x, (float)pos.y);
    TTF_DestroyText(t);
}

static void ui_sdl3_update_cursor_position(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    // TODO consider use size_t for those variables
    size_t cell_w = int_to_size(ui->cell_size.width);
    size_t cell_h = int_to_size(ui->cell_size.height);
    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        Cursor* cursor = &file_view->cursors.items[i];
        Line* line = &file_view->file->lines.items[cursor->line];
        // Compute x offset of the cursor in the line
        int line_w = 0;
        if (cursor->byte != 0) {
            TTF_MeasureString(
                ui->font_editor.main,
                file_view->file->lines.items[cursor->line].items,
                cursor->byte,
                0,
                &line_w,
                NULL);
        }
        // Compute cursor width
        int cursor_w = size_to_int(cell_w);
        if (line->count != cursor->byte) {
            // cursor not at end of line (excludes also empty lines)
            TTF_MeasureString(
                ui->font_editor.main,
                &line->items[cursor->byte],
                cursor->len,
                0,
                &cursor_w,
                NULL);
        }
        cursor->on_screen = (Rect) {
            .x = group->area.x + int_to_size(line_w),
            .y = group->area.y + (cursor->line * cell_h),
            .w = int_to_size(cursor_w),
            .h = cell_h,
        };
    }
}

static void ui_sdl3_draw_cursor(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    Color cursor_color = ui->medit->config.color_theme.cursor;

    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    bool focused = medit_get_focused_file_view_group(medit) == group;

    if (focused && !ui->cursor_blink.show_cursor) {
        return;
    }

    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        const Cursor* cursor = &file_view->cursors.items[i];

        SDL_FRect cursor_frect = {
            .x = (float)(cursor->on_screen.x + int_to_size(ui->line_nr_padding)
                         - file_view->scrolling.x),
            .y = (float)(cursor->on_screen.y - file_view->scrolling.y),
            .w = (float)cursor->on_screen.w,
            .h = (float)cursor->on_screen.h,
        };
        SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cursor_color));
        if (focused) {
            SDL_RenderFillRect(ui->renderer, &cursor_frect);
        } else {
            SDL_RenderRect(ui->renderer, &cursor_frect);
        }

        // Redraw glyph at cursor on top of it
        Line* current_line = &file_view->file->lines.items[cursor->line];
        if (cursor->byte < current_line->count) {
            const char* grapheme = &current_line->items[cursor->byte];
            PixelPos char_pos = {
                .x = size_to_int(cursor->on_screen.x) + ui->line_nr_padding
                    - size_to_int(file_view->scrolling.x),
                .y = size_to_int(cursor->on_screen.y - file_view->scrolling.y),
            };
            ui_sdl3_draw_text(
                ui,
                grapheme,
                cursor->len,
                &ui->font_editor,
                char_pos,
                color_inverse(cursor_color));
        }
    }
}

static void ui_sdl3_scroll_file_view(SDL3Ui* ui, FileViewGroup* group)
{
    FileView* file_view = medit_get_displayed_file_view_in_group(ui->medit, group);
    Cursor* cursor = &file_view->cursors.items[0];

    const size_t margin_x = int_to_size(ui->cell_size.width) * 3;
    const size_t margin_y = int_to_size(ui->cell_size.height) * 3;

    const size_t right_border = group->area.x + group->area.w - margin_x
        - int_to_size(ui->line_nr_padding);
    const size_t bottom_border = group->area.y + group->area.h - margin_y;
    const size_t left_border = group->area.x + margin_x;
    const size_t top_border = group->area.y + margin_y;

    const size_t cursor_right = cursor->on_screen.x + cursor->on_screen.w;
    const size_t cursor_bottom = cursor->on_screen.y + cursor->on_screen.h;

    // Compute the valid scroll range that keeps the cursor within both margins:
    // smallest offset that prevents the cursor from going past the right/bottom margin
    const size_t scroll_min_x = SDL_max(cursor_right, right_border) - right_border;
    const size_t scroll_min_y = SDL_max(cursor_bottom, bottom_border) - bottom_border;
    // largest offset before the cursor goes past the left/top margin
    const size_t scroll_max_x = SDL_max(cursor->on_screen.x, left_border) - left_border;
    const size_t scroll_max_y = SDL_max(cursor->on_screen.y, top_border) - top_border;

    file_view->scrolling.x = SDL_clamp(file_view->scrolling.x, scroll_min_x, scroll_max_x);
    file_view->scrolling.y = SDL_clamp(file_view->scrolling.y, scroll_min_y, scroll_max_y);
}

static void ui_sdl3_render(SDL3Ui* ui)
{
    SDL_RenderPresent(ui->renderer);
}

static void ui_sdl3_compute_line_number_gutter_width(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    const int line_count = SDL_max(size_to_int(file_view->file->lines.count), 1000);
    ui->line_nr_max_digits = digits_count(line_count);

    char buffer[INT64_DIGITS_COUNT] = { 0 };
    int written = snprintf(buffer, sizeof(buffer), "%d ", line_count);
    assert(written > 0);

    int line_number_width = 0;
    TTF_MeasureString(
        ui->font_editor.main,
        buffer,
        int_to_size(written),
        0,
        &line_number_width,
        NULL);
    assert(line_number_width >= 0);

    ui->line_nr_padding = line_number_width;
}

static void ui_sdl3_draw_line_number(
    SDL3Ui* ui,
    size_t row,
    FileViewGroup* group,
    CachedLine* cached)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    bool focused = medit_get_focused_file_view_group(medit) == group;

    const Color line_number_color = focused && row == file_view->cursors.items[0].line
        ? medit->config.color_theme.line_number_current
        : medit->config.color_theme.line_number;

    PixelPos pos = {
        .x = size_to_int(group->area.x),
        .y = size_to_int((row * int_to_size(ui->cell_size.height)) + group->area.y)
            - size_to_int(file_view->scrolling.y),
    };

    char line_number[INT64_DIGITS_COUNT] = { 0 };
    int written = snprintf(
        line_number,
        sizeof(line_number),
        "%*zu",
        ui->line_nr_max_digits,
        row + 1);
    size_t line_number_len = int_to_size(written);

    // Update cached TTF_Text for this line number if the content changed
    int cur_nr = (int)(row + 1);
    if (cached->nr_text == NULL || cached->cached_nr != cur_nr
        || cached->cached_nr_digits != ui->line_nr_max_digits) {
        if (cached->nr_text == NULL) {
            cached->nr_text = TTF_CreateText(
                ui->text_engine,
                ui->font_editor.main,
                line_number,
                line_number_len);
        } else {
            TTF_SetTextString(cached->nr_text, line_number, line_number_len);
        }
        assert(cached->nr_text != NULL);
        cached->cached_nr = cur_nr;
        cached->cached_nr_digits = ui->line_nr_max_digits;
    }

    assert(TTF_SetTextColor(cached->nr_text, color_to_RGBA_args(line_number_color)));

    const SDL_Rect clipping_rect = {
        .x = size_to_int(group->area.x),
        .y = size_to_int(group->area.y),
        .w = ui->line_nr_padding,
        .h = size_to_int(group->area.h),
    };
    assert(SDL_SetRenderClipRect(ui->renderer, &clipping_rect));

    TTF_DrawRendererText(cached->nr_text, (float)pos.x, (float)pos.y);

    assert(SDL_SetRenderClipRect(ui->renderer, NULL));
}

static void ui_sdl3_draw_line(
    SDL3Ui* ui,
    size_t row,
    Line* line,
    FileViewGroup* group,
    CachedLine* cached)
{
    if (line->count == 0) {
        return;
    }

    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    PixelPos line_pos = {
        .x = size_to_int(group->area.x) + ui->line_nr_padding - size_to_int(file_view->scrolling.x),
        .y = size_to_int((row * int_to_size(ui->cell_size.height)) + group->area.y)
            - size_to_int(file_view->scrolling.y),
    };

    // Update cached TTF_Text if the line content changed
    if (cached->text == NULL || cached->line_ptr != line->items
        || cached->line_count != line->count) {
        if (cached->text == NULL) {
            cached->text = TTF_CreateText(
                ui->text_engine,
                ui->font_editor.main,
                line->items,
                line->count);
        } else {
            TTF_SetTextString(cached->text, line->items, line->count);
        }
        assert(cached->text != NULL);
        cached->line_ptr = line->items;
        cached->line_count = line->count;
    }

    assert(TTF_SetTextColor(cached->text, color_to_RGBA_args(medit->config.color_theme.editor_fg)));

    const SDL_Rect clipping_rect = {
        .x = size_to_int(group->area.x) + ui->line_nr_padding,
        .y = size_to_int(group->area.y),
        .w = size_to_int(group->area.w) - ui->line_nr_padding,
        .h = size_to_int(group->area.h),
    };
    assert(SDL_SetRenderClipRect(ui->renderer, &clipping_rect));

    TTF_DrawRendererText(cached->text, (float)line_pos.x, (float)line_pos.y);

    assert(SDL_SetRenderClipRect(ui->renderer, NULL));
}

static void ui_sdl3_draw_file_view_group(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    Lines* lines = &file_view->file->lines;

    size_t group_idx = (size_t)(group - medit->file_views.items);
    ui_sdl3_ensure_group_caches(ui, group_idx);
    ui_sdl3_sync_group_cache(ui, group_idx, file_view);
    LineCache* cache = &ui->group_caches[group_idx];

    const size_t first_rendered_line = (file_view->scrolling.y / int_to_size(ui->cell_size.height));
    const size_t last_rendered_line = SDL_min(
        lines->count,
        (file_view->scrolling.y + int_to_size(ui->cell_size.height) + group->area.h)
            / int_to_size(ui->cell_size.height));
    for (size_t row = first_rendered_line; row < last_rendered_line; ++row) {
        Line* line = &lines->items[row];
        CachedLine* cached_line = &cache->items[row];
        ui_sdl3_draw_line_number(ui, row, group, cached_line);
        ui_sdl3_draw_line(ui, row, line, group, cached_line);
    }
}

static void ui_sdl3_draw_file_view_group_separator(SDL3Ui* ui, FileViewGroup* group)
{
    const SDL_FRect vertical_line = {
        .x = (float)group->area.x,
        .y = (float)group->area.y,
        .w = (float)group->area.w,
        .h = (float)group->area.h,
    };
    SDL_SetRenderDrawColor(
        ui->renderer,
        color_to_RGBA_args(ui->medit->config.color_theme.line_number));
    SDL_RenderRect(ui->renderer, &vertical_line);
}

// TODO temporary function placing groups on screen till we have a proper layout engine
static void temp_ui_sdl3_update_file_view_groups_size(SDL3Ui* ui)
{
    FileViewGroups* groups = &ui->medit->file_views;

    assert(groups->count > 0 && "You forgot to create some demo group");

    size_t cols = 1;
    while (cols * cols < groups->count) {
        cols++;
    }
    size_t rows = (groups->count + cols - 1) / cols;
    size_t group_width = int_to_size(ui->window_size.width) / cols;
    size_t group_height = int_to_size(ui->window_size.height) / rows;

    for (size_t i = 0; i < groups->count; ++i) {
        size_t col = i % cols;
        size_t row = i / cols;
        groups->items[i].area = (Rect) {
            .x = col * group_width,
            .y = row * group_height,
            .w = group_width,
            .h = group_height,
        };
        printf(
            "[DEBUG] group %zu: x=%zu, y=%zu, w=%zu, h=%zu\n",
            i,
            groups->items[i].area.x,
            groups->items[i].area.y,
            groups->items[i].area.w,
            groups->items[i].area.h);
    }
}

void temp_ui_sdl3_setup_layout(SDL3Ui* ui)
{
    Meditor* medit = ui->medit;

    medit_load_file(medit, "./src/ui/sdl3/sdl3.c");

    // // Create an empty file in some file view groups
    // for (size_t i = 0; i < 1; ++i) {
    //     dynarray_append(&medit->file_views, (FileViewGroup) { 0 });
    //     medit->file_views.focused = medit->file_views.count - 1;
    //     medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
    // }

    // // Insert some text in the focused latest created group
    // const char text[] = "😊😊😊😊😊😊ùùùù😊";
    // medit_insert_text(medit, text, strlen(text));

    // Update the layout of the groups in a grid fashion
    temp_ui_sdl3_update_file_view_groups_size(ui);
}

void medit_ui_sdl3_run(Meditor* medit)
{
    SDL3Ui ui = { 0 };
    assert(ui_sdl3_create(&ui, medit));

    ui_sdl3_load_editor_font(&ui);

    temp_ui_sdl3_setup_layout(&ui);

    for (size_t i = 0; i < medit->file_views.count; ++i) {
        FileViewGroup* group = &medit->file_views.items[i];
        for (size_t j = 0; j < group->count; ++j) {
            FileView* file_view = &group->items[j];
            Cursor* cursor = &file_view->cursors.items[0];
            Line* line = &file_view->file->lines.items[0];

            UcGraphemeIter it = { 0 };
            uc_grapheme_iter_init(&it, (uint8_t*)line->items, line->count, cursor->byte);
            UcSpan out = { 0 };
            uc_grapheme_iter_next(&it, &out);
            cursor->len = out.len;
            printf("group %zu, fileview %zu, cursor->len=%zu\n", i, j, cursor->len);
        }
    }

    medit->running = true;
    medit->input_in_frame = true;
    while (medit->running) {
        ui_sdl3_handle_event(&ui);
        if (medit->input_in_frame) {

            if (ui.editor_font_size != medit->config.editor_font_size) {
                ui_sdl3_unload_editor_font(&ui);
                ui_sdl3_load_editor_font(&ui);
            }

            ui_sdl3_clear(&ui);

            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                { // TODO consider doing this on event instead of every frame
                    ui_sdl3_compute_line_number_gutter_width(&ui, group);
                    ui_sdl3_update_cursor_position(&ui, group);
                    ui_sdl3_scroll_file_view(&ui, group);
                }

                ui_sdl3_draw_file_view_group_separator(&ui, group);
                ui_sdl3_draw_file_view_group(&ui, group);
                ui_sdl3_draw_cursor(&ui, group);
            }
        }
        medit->input_in_frame = false;
        ui_sdl3_render(&ui);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
