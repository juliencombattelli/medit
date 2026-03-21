#include "sdl3.h"
#include "assert.h"
#include "dynarray.h"
#include "font.h"
#include "keybind.h"
#include "meditor.h"
#include "utf8.h"
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
    Meditor* medit;
    SDL_Window* window;
    SDL_Renderer* renderer;
    Font font_ui;
    Font font_editor;
    PixelSize cell_size;
    PixelSize window_size;
    CursorBlink cursor_blink;
    int line_nr_padding;
    int editor_font_size;
} SDL3Ui;

static bool ui_sdl3_create(SDL3Ui* ui, Meditor* medit);
static void ui_sdl3_destroy(SDL3Ui* ui);

static void ui_sdl3_resize_window_with_data(SDL3Ui* ui, PixelSize window_size);
static void ui_sdl3_resize_window(SDL3Ui* ui);
static void ui_sdl3_resize_cell(SDL3Ui* ui);
static void ui_sdl3_resize_grid(SDL3Ui* ui);

static void ui_sdl3_load_ui_font(SDL3Ui* ui);
static void ui_sdl3_unload_ui_font(SDL3Ui* ui);
static void ui_sdl3_load_editor_font(SDL3Ui* ui);
static void ui_sdl3_unload_editor_font(SDL3Ui* ui);

static void ui_sdl3_handle_event(SDL3Ui* ui);

static void ui_sdl3_clear(SDL3Ui* ui);
static void ui_sdl3_draw_debug_grid(SDL3Ui* ui, FileViewGroup* group);
static void ui_sdl3_draw_text(
    SDL3Ui* ui,
    const char* text,
    size_t len,
    Font* font,
    PixelPos pos,
    Color color);
static void ui_sdl3_draw_cursor(SDL3Ui* ui, FileViewGroup* group);
static void ui_sdl3_render(SDL3Ui* ui);

static PixelPos cell_to_pixel_pos(SDL3Ui* ui, Cell cell)
{
    assert(cell.col <= INT_MAX);
    assert(cell.row <= INT_MAX);
    return (PixelPos) {
        .x = (int)cell.col * ui->cell_size.width,
        .y = (int)cell.row * ui->cell_size.height,
    };
}

enum {
    DEFAULT_WINDOW_WIDTH = 1280,
    DEFAULT_WINDOW_HEIGHT = 720,
    DEFAULT_CURSOR_BLINK_PERIOD_MS = 1000,
};

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

    try(SDL_SetRenderVSync(renderer, 1));

    ui->window = window;
    ui->renderer = renderer;

    try(SDL_ShowWindow(ui->window));

    try(SDL_StartTextInput(ui->window));

    medit_load_default_gui_keybind(medit);

    ui->medit = medit;

    return true;
}

static void ui_sdl3_destroy(SDL3Ui* ui)
{
    SDL_StopTextInput(ui->window);

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

static void ui_sdl3_resize_grid(SDL3Ui* ui)
{
    ui->medit->grid_size = (Cell) {
        .col = (size_t)ui->window_size.width / (size_t)ui->cell_size.width,
        .row = (size_t)ui->window_size.height / (size_t)ui->cell_size.height,
    };
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
    ui_sdl3_resize_grid(ui);

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
    ui_sdl3_resize_grid(ui);
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
    const size_t text_cells = 1; // medit_get_text_cells(medit, event.text.text);
    size_t text_len = strlen(text);
    medit_insert_text(ui->medit, text, text_len, text_cells);
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
                ui_sdl3_on_window_resized(ui, (int)event.window.data1, (int)event.window.data2);
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

static void ui_sdl3_draw_debug_grid(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    if (!medit->draw_debug_grid) {
        return;
    }

    SDL_SetRenderDrawColor(ui->renderer, 255, 0, 255, 255);

    assert(medit->grid_size.col <= INT_MAX);
    assert(group->offset <= INT_MAX);
    for (int i = 0; i < (int)medit->grid_size.col + 1; ++i) {
        const SDL_FRect vertical_line = {
            .x = (float)(i * ui->cell_size.width + (int)group->offset + ui->line_nr_padding),
            .y = (float)0,
            .w = (float)1,
            .h = (float)ui->window_size.height,
        };
        SDL_RenderRect(ui->renderer, &vertical_line);
    }

    assert(medit->grid_size.row <= INT_MAX);
    for (int i = 0; i < (int)medit->grid_size.row + 1; ++i) {
        const SDL_FRect horizontal_line = {
            .x = (float)0,
            .y = (float)(i * ui->cell_size.height),
            .w = (float)ui->window_size.width,
            .h = (float)1,
        };
        SDL_RenderRect(ui->renderer, &horizontal_line);
    }
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

    // Take one extra cell to allow display partial chars
    int max_string_width = ui->window_size.width + ui->cell_size.width;

    int text_width = 0;
    size_t text_bytes_max = 0;
    bool res = TTF_MeasureString(
        font->main,
        text,
        len,
        max_string_width,
        &text_width,
        &text_bytes_max);
    assert(res == true);

    size_t printed_bytes = SDL_min((size_t)len, text_bytes_max);

    if (text_width == 0) {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(
        font->main,
        text,
        printed_bytes,
        color_to_sdl3(color));
    assert(surface != NULL);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(ui->renderer, surface);
    assert(texture != NULL);

    const SDL_FRect glyph_rect = {
        .x = (float)pos.x,
        .y = (float)pos.y,
        .w = (float)surface->w,
        .h = (float)surface->h,
    };

    SDL_RenderTexture(ui->renderer, texture, NULL, &glyph_rect);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

static void ui_sdl3_draw_cursor(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    Color cursor_color = ui->medit->config.color_theme.cursor;

    FileView* file_view = medit_get_focused_file_view(medit);
    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        FileCoord* cursor = &file_view->cursors.items[i].in_file;

        assert(cursor->byte <= INT_MAX);
        assert(cursor->line <= INT_MAX);
        assert(group->offset <= INT_MAX);

        const SDL_FRect cursor_rect = {
            .x = (float)((int)group->offset + ui->line_nr_padding
                         + ((int)cursor->byte * ui->cell_size.width)),
            .y = (float)((int)cursor->line * ui->cell_size.height - file_view->scroll_y),
            .w = (float)(ui->cell_size.width),
            .h = (float)(ui->cell_size.height),
        };

        // TODO take group->offset.y + group->size.h
        if (cursor_rect.y >= ui->window_size.height - ui->cell_size.height) {
            printf("Cursor out of view\n");
            // TODO compute the height to scroll instead of moving just one cell up/down
            file_view->scroll_y += ui->cell_size.height;
        }

        SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cursor_color));
        SDL_RenderFillRect(ui->renderer, &cursor_rect);

        // Redraw char at cursor on top of it
        Line* current_line = &file_view->file->lines.items[cursor->line];
        if (cursor->byte < current_line->count) {
            const char* c = &current_line->items[cursor->byte];
            Cell cell = {
                .col = cursor->byte,
                .row = cursor->line,
            };
            PixelPos char_pos = cell_to_pixel_pos(ui, cell);
            char_pos.x += (int)group->offset + ui->line_nr_padding;
            char_pos.y -= file_view->scroll_y;
            ui_sdl3_draw_text(ui, c, 1, &ui->font_editor, char_pos, color_inverse(cursor_color));
        }
    }
}

// static void ui_sdl3_draw_cursor_utf8(SDL3Ui* ui, FileViewGroup* group)
// {
//     Meditor* medit = ui->medit;

//     Color cursor_color = ui->medit->config.color_theme.cursor;

//     FileView* file_view = medit_get_focused_file_view(medit);
//     for (size_t i = 0; i < file_view->cursors.count; ++i) {
//         FileCoord* fc = &file_view->cursors.items[i].in_file;

//         // Convert byte offset to codepoint index for visual column placement
//         Line* current_line = &file_view->file->lines.items[fc->line];
//         Cell visual = {
//             .col = utf8_byte_to_codepoint_index(current_line->items, fc->byte),
//             .row = fc->line,
//         };

//         assert(visual.col <= INT_MAX);
//         assert(visual.row <= INT_MAX);
//         assert(group->offset <= INT_MAX);

//         const SDL_FRect cursor_rect = {
//             .x = (float)((int)group->offset + ui->line_nr_padding
//                          + ((int)visual.col * ui->cell_size.width)),
//             .y = (float)((int)visual.row * ui->cell_size.height),
//             .w = (float)(ui->cell_size.width),
//             .h = (float)(ui->cell_size.height),
//         };

//         SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cursor_color));
//         SDL_RenderFillRect(ui->renderer, &cursor_rect);

//         // Redraw char at cursor on top of it
//         if (fc->byte < current_line->count) {
//             size_t cp_size = utf8_codepoint_size((unsigned char)current_line->items[fc->byte]);
//             const char* c = &current_line->items[fc->byte];
//             PixelPos char_pos = cell_to_pixel_pos(ui, visual);
//             char_pos.x += (int)group->offset + ui->line_nr_padding;
//             ui_sdl3_draw_text(
//                 ui,
//                 c,
//                 cp_size,
//                 &ui->font_editor,
//                 char_pos,
//                 color_inverse(cursor_color));
//         }
//     }
// }

static void ui_sdl3_render(SDL3Ui* ui)
{
    SDL_RenderPresent(ui->renderer);
}

static size_t format_line_number(SDL3Ui* ui, size_t line_number, char* buffer, size_t bufsize)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_focused_file_view(medit);

    // Compute the maximum number of digits (minimum 4)
    const size_t line_count = SDL_max(file_view->file->lines.count, 1000);
    const int max_digits = digits_count((int)line_count);

    { // Compute the padding size to render this max line number
        int written = snprintf(buffer, bufsize, "%zu ", line_count);
        assert(written > 0);

        int line_number_width = 0;
        TTF_MeasureString(
            ui->font_editor.main,
            buffer,
            (size_t)written,
            0,
            &line_number_width,
            NULL);
        assert(line_number_width >= 0);

        ui->line_nr_padding = line_number_width;
    }

    // Render the number of the current line
    int written = snprintf(buffer, bufsize, "%*zu", max_digits, line_number + 1);
    assert(written > 0);
    return (size_t)written;
}

static void ui_sdl3_draw_line_number(SDL3Ui* ui, size_t row, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_focused_file_view(medit);

    bool focused = medit_get_focused_file_view_group(medit) == group;

    const Color line_number_color = focused && row == file_view->cursors.items[0].in_file.line
        ? medit->config.color_theme.line_number_current
        : medit->config.color_theme.line_number;

    assert(group->offset <= INT_MAX);

    PixelPos pos = cell_to_pixel_pos(ui, (Cell) { .col = 0, .row = row });
    pos.x += (int)group->offset;
    pos.y -= file_view->scroll_y;

    char line_number[64]; // size big enough to hold the number of digits in a 64 bits integer
    size_t line_numnber_len = format_line_number(ui, row, line_number, sizeof(line_number));
    ui_sdl3_draw_text(ui, line_number, line_numnber_len, &ui->font_editor, pos, line_number_color);
}

static void ui_sdl3_draw_line(SDL3Ui* ui, size_t row, Line* line, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_focused_file_view(medit);

    assert(group->offset <= INT_MAX);

    PixelPos line_pos = cell_to_pixel_pos(ui, (Cell) { .col = 0, .row = row });
    line_pos.x += (int)group->offset + ui->line_nr_padding;
    line_pos.y -= file_view->scroll_y;

    ui_sdl3_draw_text(
        ui,
        line->items,
        line->count,
        &ui->font_editor,
        line_pos,
        medit->config.color_theme.editor_fg);
}

static void ui_sdl3_update_file_view_groups_size(SDL3Ui* ui)
{
    size_t offset = 0;
    FileViewGroups* groups = &ui->medit->file_views;
    dynarray_foreach(FileViewGroup, group, groups)
    {
        group->offset = offset;
        group->width = (size_t)ui->window_size.width / groups->count;
        offset += group->width;
    }
}

static void ui_sdl3_draw_file_view_group(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    Lines* lines = &file_view->file->lines;
    for (size_t i = 0; i < lines->count; ++i) {
        if (i < file_view->scroll_y / ui->cell_size.height) {
            continue;
        }
        if (i > file_view->scroll_y / ui->cell_size.height + ui->window_size.height) {
            continue;
        }
        Line* line = &lines->items[i];
        ui_sdl3_draw_line_number(ui, i, group);
        ui_sdl3_draw_line(ui, i, line, group);
    }
}

static void ui_sdl3_draw_file_view_group_separator(SDL3Ui* ui, FileViewGroup* group, size_t i)
{
    const SDL_FRect vertical_line = {
        .x = (float)(i * (size_t)ui->cell_size.width + group->offset),
        .y = (float)0,
        .w = (float)1,
        .h = (float)ui->window_size.height,
    };
    SDL_SetRenderDrawColor(
        ui->renderer,
        color_to_RGBA_args(ui->medit->config.color_theme.line_number));
    SDL_RenderRect(ui->renderer, &vertical_line);
}

void medit_ui_sdl3_run(Meditor* medit)
{
    SDL3Ui ui = { 0 };
    assert(ui_sdl3_create(&ui, medit));

    ui_sdl3_load_editor_font(&ui);

    medit->running = true;
    medit->input_in_frame = true;
    while (medit->running) {
        ui_sdl3_handle_event(&ui);

        if (ui.editor_font_size != medit->config.editor_font_size) {
            ui_sdl3_unload_editor_font(&ui);
            ui_sdl3_load_editor_font(&ui);
        }

        ui_sdl3_update_file_view_groups_size(&ui);

        if (!medit->input_in_frame) {
            continue;
        }
        medit->input_in_frame = false;

        ui_sdl3_clear(&ui);

        for (size_t i = 0; i < medit->file_views.count; ++i) {
            FileViewGroup* group = &medit->file_views.items[i];
            if (i != 0) {
                ui_sdl3_draw_file_view_group_separator(&ui, group, i);
            }
            ui_sdl3_draw_file_view_group(&ui, group);
            ui_sdl3_draw_debug_grid(&ui, group);
            if (medit->file_views.focused == i && ui.cursor_blink.show_cursor) {
                ui_sdl3_draw_cursor(&ui, group);
            }
        }

        ui_sdl3_render(&ui);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
