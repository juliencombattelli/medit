#include "sdl3.h"
#include "action.h"
#include "assert.h"
#include "dynarray.h"
#include "font.h"
#include "keybind.h"
#include "meditor.h"
#include "perf_counter.h"
#include "rect.h"
#include "safeint.h"
#include "sdl3_utils.h"
#include "ui.h"
#include "unicode.h"
#include "utils.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <limits.h>

enum {
    UI_TEXT_ARENA_SIZE = 4096,
};

typedef struct {
    int width;
    int height;
} PixelSize;

typedef struct {
    int x;
    int y;
} PixelPos;

typedef struct {
    SDL_TimerID timer;
    bool show;
} CursorBlinker;

typedef struct {
    SDL_PropertiesID props;
    TTF_Font* main;
    TTF_Font* emoji;
    size_t line_spacing;
    size_t default_cursor_width;
    int line_centering_offset;
} Font;

typedef struct {
    Meditor* medit;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Text* text_cache;
    Font font_ui;
    Font font_editor;
    PixelSize window_size;
    CursorBlinker cursor_blinker;
    PerfCounter perf_counter;
    Layout layout;
    int line_nr_padding;
    int line_nr_max_digits;
    int line_nr_cached_line_count;
    int editor_font_size;
    // UI module - 5 draw layers
    // Layer 1 (bg):            panels, backgrounds, tab bars
    // Layer 2 (file content):  file lines, line numbers, decorations
    // Layer 3 (overlay):       cursors
    // Layer 4 (cursor glyph):  redraw the glyphs behind cursors on top of them
    // Layer 5 (popups):        all popups and floating panels (not yet implemented)
    UiCtx ui_ctx_bg;
    UiCtx ui_ctx_overlay;
    UiDrawCmdList ui_draw_list_bg;
    UiDrawCmdList ui_draw_list_overlay;
    // Per-frame string arena for UI_CMD_TEXT labels (reset each frame alongside draw lists)
    char ui_text_arena[UI_TEXT_ARENA_SIZE];
    size_t ui_text_arena_used;
    // Per-frame scroll delta accumulator (reset each frame)
    float ui_scroll_delta_x;
    float ui_scroll_delta_y;
    bool ui_scroll_valid;
    // Mouse state from previous frame, to detect clicks (press+release)
    bool ui_mouse_was_down;
} SDL3Ui;

enum {
    DEFAULT_WINDOW_WIDTH = 1280,
    DEFAULT_WINDOW_HEIGHT = 720,
    DEFAULT_CURSOR_BLINK_MS = 500,
};

enum {
    WAIT_FOR_EVENT_TIMEOUT_MS = 100,
    PERF_COUNTER_REPORT_PERIOD_MS = 1000,
};

enum UserEvents {
    EVENT_CURSOR_BLINK = 42,
};

static inline void ui_sdl3_recompute_layout(SDL3Ui* ui)
{
    medit_layout_recompute(
        &ui->layout,
        &ui->medit->file_views,
        int_to_size(ui->window_size.width),
        int_to_size(ui->window_size.height));
}

static void action_dump_state(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_dump_state(medit);
}

static void action_save_file(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_save_file(medit);
}

static void ui_sdl3_open_file_dialog_cb(void* userdata, const char* const* filelist, int filter)
{
    MEDIT_UNUSED(filter);

    SDL3Ui* ui = userdata;

    if (!filelist) {
        (void)fprintf(stderr, "Error: %s\n", SDL_GetError());
        return;
    }
    if (!*filelist) {
        printf("The user did not select any file.\n");
        return;
    }
    while (*filelist) {
        medit_load_file(ui->medit, *filelist);
        filelist++;
    }
}

static void action_open_file_dialog(Meditor* medit, void* ui_)
{
    MEDIT_UNUSED(medit);

    SDL3Ui* ui = ui_;
    SDL_ShowOpenFileDialog(ui_sdl3_open_file_dialog_cb, ui, NULL, NULL, 0, NULL, 1);
}

static void action_toggle_side_panel(Meditor* medit, void* ui_)
{
    MEDIT_UNUSED(medit);

    SDL3Ui* ui = ui_;
    medit_layout_toggle_shown_element(&ui->layout, LAYOUT_SIDE_PANEL);
    ui_sdl3_recompute_layout(ui);
}

static const Actions UI_SDL3_ACTIONS = {
    MEDIT_CORE_ACTIONS_DEFAULT,
    .save_file = action_save_file,
    .open_file_dialog = action_open_file_dialog,
    .toggle_side_panel = action_toggle_side_panel,
    .dump_state = action_dump_state,
};

static const LayoutSizes UI_SDL3_DEFAULT_LAYOUT_SIZES = {
    .menu_bar_height = 30,
    .tab_bar_height = 35,
    .side_panel_width = 200,
    .bottom_panel_height = 200,
    .status_bar_height = 30,
    .separator_size = 1,
};

static bool ui_sdl3_create(SDL3Ui* ui, Meditor* medit)
{
    try(SDL_Init(SDL_INIT_VIDEO));
    try(TTF_Init());

    SDL_Window* window = SDL_CreateWindow(
        "Medit",
        DEFAULT_WINDOW_WIDTH,
        DEFAULT_WINDOW_HEIGHT,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    try(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    try(renderer);

    printf("[DEBUG] Selected renderer: %s\n", SDL_GetRendererName(renderer));

    // try(SDL_SetRenderVSync(renderer, 1));
    try(SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED));

    TTF_TextEngine* text_engine = TTF_CreateRendererTextEngine(renderer);
    try(text_engine);

    ui->medit = medit;
    ui->window = window;
    ui->renderer = renderer;
    ui->text_engine = text_engine;

    try(SDL_ShowWindow(ui->window));

    try(SDL_StartTextInput(ui->window));

    medit_load_default_keybind_full(medit, &UI_SDL3_ACTIONS, ui);

    ui_draw_cmd_list_init(&ui->ui_draw_list_bg);
    ui_draw_cmd_list_init(&ui->ui_draw_list_overlay);

    return true;
}

static void ui_sdl3_destroy(SDL3Ui* ui)
{
    SDL_StopTextInput(ui->window);

    ui_draw_cmd_list_free(&ui->ui_draw_list_bg);
    ui_draw_cmd_list_free(&ui->ui_draw_list_overlay);

    TTF_DestroyRendererTextEngine(ui->text_engine);
    SDL_DestroyRenderer(ui->renderer);
    SDL_DestroyWindow(ui->window);

    TTF_Quit();
    SDL_Quit();

    *ui = (SDL3Ui) { 0 };
}

// Allocate a null-terminated string copy from the per-frame arena
// Returns NULL if the arena is full (the label is silently dropped)
// TODO reallocate a new arena if full? or used buckets?
static const char* ui_sdl3_arena_str(SDL3Ui* ui, const char* str, size_t len)
{
    size_t needed = len + 1;
    if (ui->ui_text_arena_used + needed > UI_TEXT_ARENA_SIZE) {
        printf(
            "Error: cannot allocate str from arena, capacity: %zu, total needed: %zu\n",
            (size_t)UI_TEXT_ARENA_SIZE,
            ui->ui_text_arena_used + needed);
        return NULL;
    }
    char* dst = &ui->ui_text_arena[ui->ui_text_arena_used];
    memcpy(dst, str, len);
    dst[len] = '\0';
    ui->ui_text_arena_used += needed;
    return dst;
}

static void ui_sdl3_draw_frame_begin(SDL3Ui* ui)
{
    ui_draw_cmd_list_clear(&ui->ui_draw_list_bg);
    ui_draw_cmd_list_clear(&ui->ui_draw_list_overlay);
    ui->ui_text_arena_used = 0;

    float mouse_x = 0.f;
    float mouse_y = 0.f;
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    bool is_down = (buttons & SDL_BUTTON_LMASK) != 0;

    ui->ui_ctx_bg = (UiCtx) {
        .draw_list    = &ui->ui_draw_list_bg,
        .scroll_speed = (float)ui->font_editor.line_spacing,
        .input = {
            .x            = (size_t)mouse_x,
            .y            = (size_t)mouse_y,
            .left_down    = is_down,
            .left_clicked = ui->ui_mouse_was_down && !is_down,
            .scroll_x     = ui->ui_scroll_delta_x,
            .scroll_y     = ui->ui_scroll_delta_y,
            .scroll_valid = ui->ui_scroll_valid,
        },
    };
    ui->ui_ctx_overlay = (UiCtx) {
        .draw_list    = &ui->ui_draw_list_overlay,
        .scroll_speed = (float)ui->font_editor.line_spacing,
        .input = {
            .x            = (size_t)mouse_x,
            .y            = (size_t)mouse_y,
            .left_down    = is_down,
            .left_clicked = ui->ui_mouse_was_down && !is_down,
            .scroll_x     = ui->ui_scroll_delta_x,
            .scroll_y     = ui->ui_scroll_delta_y,
            .scroll_valid = ui->ui_scroll_valid,
        },
    };

    ui->ui_scroll_delta_x = 0;
    ui->ui_scroll_delta_y = 0;
    ui->ui_scroll_valid = false;
    ui->ui_mouse_was_down = is_down;
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

    if (!TTF_FontIsFixedWidth(ui->font_editor.main)) {
        printf(
            "[WARN] The loaded editor font is not monospace: %s",
            TTF_GetFontFamilyName(ui->font_editor.main));
    }

    int line_spacing = TTF_GetFontLineSkip(ui->font_editor.main);
    ui->font_editor.line_spacing = int_to_size(line_spacing);
    int font_h = TTF_GetFontHeight(ui->font_editor.main);
    ui->font_editor.line_centering_offset = (line_spacing - font_h + 1) / 2;

    int w = 0;
    assert(TTF_GetStringSize(ui->font_editor.main, "M", 0, &w, NULL));
    ui->font_editor.default_cursor_width = int_to_size(w);

    ui_sdl3_resize_window(ui);

    ui->text_cache = TTF_CreateText(ui->text_engine, ui->font_editor.main, "", 0);
    assert(ui->text_cache != NULL);

    const int width_factor = 0; // Do not align the emoji font width to the main font width
    ui->font_editor.emoji = load_emoji_font_aligned_to(
        ui->font_editor.main,
        // "asset/font/NotoColorEmoji-Regular.ttf",
        "asset/font/OpenMoji-color-colr0_svg.ttf",
        // "asset/font/seguiemj.ttf",
        medit->config.editor_font_size,
        width_factor);
    if (!ui->font_editor.emoji) {
        printf("Warning: failed to load fallback emoji font: %s\n", SDL_GetError());
    } else {
        if (!TTF_AddFallbackFont(ui->font_editor.main, ui->font_editor.emoji)) {
            printf("Warning: failed to load fallback emoji font: %s\n", SDL_GetError());
            abort();
        }
    }
}

static void ui_sdl3_unload_editor_font(SDL3Ui* ui)
{
    TTF_DestroyText(ui->text_cache);
    ui->text_cache = NULL;

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

    ui_sdl3_recompute_layout(ui);
}

static Uint32 ui_sdl3_on_cursor_should_blink(void* userdata, SDL_TimerID timer_id, Uint32 interval)
{
    MEDIT_UNUSED(timer_id);
    MEDIT_UNUSED(interval);

    SDL3Ui* ui = userdata;
    ui->cursor_blinker.show = !ui->cursor_blinker.show;

    Uint64 now = SDL_GetTicksNS();
    SDL_Event event = {
        .user = (SDL_UserEvent) {
            .type = SDL_EVENT_USER,
            .timestamp = now,
            .code = EVENT_CURSOR_BLINK,
        },
    };
    SDL_PushEvent(&event);

    return DEFAULT_CURSOR_BLINK_MS;
}

static void ui_sdl3_enable_cursor_blink(SDL3Ui* ui)
{
    ui->cursor_blinker.show = false;
    ui->cursor_blinker.timer = SDL_AddTimer(0, ui_sdl3_on_cursor_should_blink, ui);
}

static void ui_sdl3_disable_cursor_blink(SDL3Ui* ui)
{
    SDL_RemoveTimer(ui->cursor_blinker.timer);
}

static void ui_sdl3_reset_cursor_blinking_timer_on_input(SDL3Ui* ui, SDL_Event* event)
{
    switch (event->type) {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_KEY_DOWN: {
            ui_sdl3_disable_cursor_blink(ui);
            ui_sdl3_enable_cursor_blink(ui);
        } break;
        default: break;
    }
}

static void ui_sdl3_handle_save_of_dirty_file(
    SDL3Ui* ui,
    File* file,
    SDL_MessageBoxData* messageboxdata,
    bool* cancel_exit)
{
    *cancel_exit = false;

    static const char fmt[] = "Do you want to save the changes you made to %s?";
    int format_ret = snprintf(NULL, 0, fmt, file->name) + 1; // +1 for null terminator
    size_t message_len = int_to_size(format_ret);
    char* msg = calloc(message_len, 1);
    (void)snprintf(msg, message_len, fmt, file->name);
    messageboxdata->message = msg;

    if (file->dirty) {
        int buttonid = 0;
        assert(SDL_ShowMessageBox(messageboxdata, &buttonid));
        switch (buttonid) {
            case 0:
                printf("Saving changes for file %s\n", file->name);
                medit_save_file(ui->medit);
                break;
            case 1: printf("Discarding changes for file %s\n", file->name); break;
            default: printf("Cancelling exit\n"); *cancel_exit = true;
        }
    }

    free(msg);
}

static void ui_sdl3_handle_save_of_dirty_files(SDL3Ui* ui)
{
    static const SDL_MessageBoxButtonData buttons[] = {
        {
            .flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            .buttonID = 0,
            .text = "Save",
        },
        {
            .flags = 0,
            .buttonID = 1,
            .text = "Don't Save",
        },
        {
            .flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            .buttonID = 2,
            .text = "Cancel",
        },
    };

    static SDL_MessageBoxData messageboxdata = {
        .flags = SDL_MESSAGEBOX_WARNING | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT,
        .window = NULL,
        .title = "Medit",
        .message = NULL,
        .numbuttons = SDL_arraysize(buttons),
        .buttons = buttons,
        .colorScheme = NULL,
    };

    for (size_t i = 0; i < ui->medit->opened_files.count; ++i) {
        File* file = &ui->medit->opened_files.items[i];
        bool cancel_exit = false;
        ui_sdl3_handle_save_of_dirty_file(ui, file, &messageboxdata, &cancel_exit);
        if (cancel_exit) {
            break;
        }
        ui->medit->running = false;
    }
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
        case SDLK_RETURN: {
            medit_split_line_at_cursor(ui->medit);
            medit_cursor_down(ui->medit);
            medit_cursor_line_begin(ui->medit);
            medit_file_view_file(ui->medit, medit_get_focused_file_view(ui->medit))->dirty = true;
        } break;
        case SDLK_BACKSPACE:
            medit_erase_char(ui->medit);
            medit_file_view_file(ui->medit, medit_get_focused_file_view(ui->medit))->dirty = true;
            break;
        default: break;
    }
}

static void ui_sdl3_dispatch_event(SDL3Ui* ui, SDL_Event* event)
{
    Meditor* medit = ui->medit;

    switch (event->type) {
        case SDL_EVENT_QUIT: ui_sdl3_handle_save_of_dirty_files(ui); break;
        case SDL_EVENT_WINDOW_RESIZED:
            ui_sdl3_on_window_resized(ui, event->window.data1, event->window.data2);
            break;
        case SDL_EVENT_KEY_DOWN: {
            KeybindEvent keybind_event = keybind_sdl3_translate_event(event);
            if (keybind_handle_event(&medit->keybind, &keybind_event)) {
                break;
            }
            ui_sdl3_on_key_down(ui, event);
        } break;
        case SDL_EVENT_TEXT_INPUT: {
            ui_sdl3_on_text_input(ui, event->text.text);
        } break;
        case SDL_EVENT_KEYMAP_CHANGED: {
            printf("Reloading keymapping\n");
            keybind_reinit(&medit->keybind);
            medit_load_default_keybind_full(ui->medit, &UI_SDL3_ACTIONS, ui);
        } break;
        case SDL_EVENT_MOUSE_WHEEL: {
            ui->ui_scroll_delta_x += event->wheel.x;
            ui->ui_scroll_delta_y += event->wheel.y;
            ui->ui_scroll_valid = true;
        } break;
        default: break;
    }
}

static bool ui_sdl3_handle_event(SDL3Ui* ui)
{
    // Save current font size to monitor changes
    ui->editor_font_size = ui->medit->config.editor_font_size;

    // Block until an event arrives or a timeout, saving CPU
    // Pass NULL to avoid consuming the first event, so PollEvent drains everything uniformly
    if (SDL_WaitEventTimeout(NULL, WAIT_FOR_EVENT_TIMEOUT_MS)) {
        perf_counter_frame_begin(&ui->perf_counter);
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            ui_sdl3_reset_cursor_blinking_timer_on_input(ui, &event);
            ui_sdl3_dispatch_event(ui, &event);
        }
        return true;
    }
    // Timeout (no events): nothing changed, skip render
    perf_counter_frame_discard(&ui->perf_counter);
    return false;
}

static void ui_sdl3_clear(SDL3Ui* ui)
{
    Color color = ui->medit->config.color_theme.editor_bg;
    SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(color));
    SDL_RenderClear(ui->renderer);
}

static void ui_sdl3_draw_panel(SDL3Ui* ui, Panel panel, Color bg)
{
    Color border = ui->medit->config.color_theme.panel_border;
    ui_panel(&ui->ui_ctx_bg, panel.area, bg);
    ui_panel(&ui->ui_ctx_bg, panel.separator, border);
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

    TTF_Text* text_obj = ui->text_cache;

    TTF_SetTextFont(text_obj, font->main);
    TTF_SetTextString(text_obj, text, len);
    TTF_SetTextColor(text_obj, color_to_RGBA_args(color));

    TTF_DrawRendererText(text_obj, (float)pos.x, (float)pos.y + (float)font->line_centering_offset);
}

static Rect ui_sdl3_cursor_rect(
    SDL3Ui* ui,
    Rect text_area,
    const Cursor* cursor,
    FileView* file_view)
{
    Line* line = &medit_file_view_file(ui->medit, file_view)->lines.items[cursor->line];
    // Compute x offset of the cursor in the line
    int line_w = 0;
    if (cursor->byte != 0) {
        TTF_MeasureString(
            ui->font_editor.main,
            medit_file_view_file(ui->medit, file_view)->lines.items[cursor->line].items,
            cursor->byte,
            0,
            &line_w,
            NULL);
    }
    // Compute cursor width
    int cursor_w = size_to_int(ui->font_editor.default_cursor_width);
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
    return (Rect) {
        .x = text_area.x + int_to_size(line_w),
        .y = text_area.y + (cursor->line * ui->font_editor.line_spacing),
        .w = int_to_size(cursor_w),
        .h = ui->font_editor.line_spacing,
    };
}

// Queue cursor rect(s) into the overlay draw layer
static void ui_sdl3_queue_cursor(SDL3Ui* ui, Rect text_area, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    Color cursor_color = ui->medit->config.color_theme.cursor;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    bool focused = medit_get_focused_file_view_group(medit) == group;

    if (focused && !ui->cursor_blinker.show) {
        return;
    }

    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        const Cursor* cursor = &file_view->cursors.items[i];
        const Rect on_screen = ui_sdl3_cursor_rect(ui, text_area, cursor, file_view);

        Rect cursor_rect = {
            .x = on_screen.x - file_view->scrolling.x,
            .y = on_screen.y - file_view->scrolling.y,
            .w = on_screen.w,
            .h = on_screen.h,
        };

        if (focused) {
            UiDrawCmd fill_cmd = {
                .kind = UI_CMD_RECT_FILLED,
                .rect = cursor_rect,
                .color = cursor_color,
            };
            dynarray_append(&ui->ui_draw_list_overlay, fill_cmd);
        } else {
            UiDrawCmd outline_cmd = {
                .kind = UI_CMD_RECT_OUTLINED,
                .rect = cursor_rect,
                .color = cursor_color,
                .outline_color = cursor_color,
            };
            dynarray_append(&ui->ui_draw_list_overlay, outline_cmd);
        }
    }
}

// Draw the glyph on top of each cursor rect directly
// Must be called after the overlay layer has been flushed
static void ui_sdl3_draw_cursor_glyphs(SDL3Ui* ui, Rect text_area, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    Color cursor_color = ui->medit->config.color_theme.cursor;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    bool focused = medit_get_focused_file_view_group(medit) == group;

    if (focused && !ui->cursor_blinker.show) {
        return;
    }

    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        const Cursor* cursor = &file_view->cursors.items[i];
        const Rect on_screen = ui_sdl3_cursor_rect(ui, text_area, cursor, file_view);

        Color glyph_color = focused ? color_inverse(cursor_color) : cursor_color;

        Line* current_line = &medit_file_view_file(medit, file_view)->lines.items[cursor->line];
        if (cursor->byte < current_line->count) {
            const char* grapheme = &current_line->items[cursor->byte];
            PixelPos char_pos = {
                .x = size_to_int(on_screen.x) - size_to_int(file_view->scrolling.x),
                .y = size_to_int(on_screen.y - file_view->scrolling.y),
            };
            ui_sdl3_draw_text(ui, grapheme, cursor->len, &ui->font_editor, char_pos, glyph_color);
        }
    }
}

// Enough to hold 2 64-bits integers and some text
#define CURSOR_POS_SEGMENT_LENGTH (INT64_DIGITS_COUNT * 2 + 32)

static void ui_sdl3_draw_status_bar_text(SDL3Ui* ui)
{
    FileView* file_view = medit_get_focused_file_view(ui->medit);
    Cursor* cursor = &file_view->cursors.items[0];
    size_t col = cursor->grapheme_col + 1;
    size_t line = cursor->line + 1;
    char cursor_pos_segment[CURSOR_POS_SEGMENT_LENGTH] = { 0 };
    int written = snprintf(
        cursor_pos_segment,
        sizeof(cursor_pos_segment),
        "Ln %zu, Col %zu ",
        line,
        col);
    size_t len = int_to_size(written);

    int segment_width = 0;
    TTF_MeasureString(ui->font_editor.main, cursor_pos_segment, len, 0, &segment_width, NULL);

    Panel* status_bar = &ui->layout.status_bar;
    int font_h = TTF_GetFontHeight(ui->font_editor.main);
    int text_x = size_to_int(status_bar->area.x + status_bar->area.w) - segment_width;
    int text_y = size_to_int(status_bar->area.y) + ((size_to_int(status_bar->area.h) - font_h) / 2);

    const char* status_text = ui_sdl3_arena_str(ui, cursor_pos_segment, len);
    if (!status_text) {
        return;
    }

    UiDrawCmd text_cmd = {
        .kind = UI_CMD_TEXT,
        .rect = { .x = int_to_size(text_x),
                  .y = int_to_size(text_y),
                  .w = int_to_size(segment_width),
                  .h = status_bar->area.h },
        .color = ui->medit->config.color_theme.editor_fg,
        .text = status_text,
    };
    dynarray_append(&ui->ui_draw_list_bg, text_cmd);
}

static void ui_sdl3_scroll_file_view(SDL3Ui* ui, Rect text_area, FileViewGroup* group)
{
    FileView* file_view = medit_get_displayed_file_view_in_group(ui->medit, group);
    Cursor* cursor = &file_view->cursors.items[0];
    const Rect on_screen = ui_sdl3_cursor_rect(ui, text_area, cursor, file_view);

    const size_t margin_x = ui->font_editor.default_cursor_width * 3;
    const size_t margin_y = ui->font_editor.line_spacing * 3;

    const size_t right_border = text_area.x + text_area.w - margin_x;
    const size_t bottom_border = text_area.y + text_area.h - margin_y;
    const size_t left_border = text_area.x + margin_x;
    const size_t top_border = text_area.y + margin_y;

    const size_t cursor_right = on_screen.x + on_screen.w;
    const size_t cursor_bottom = on_screen.y + on_screen.h;

    // Compute the valid scroll range that keeps the cursor within both margins:
    // smallest offset that prevents the cursor from going past the right/bottom margin
    const size_t scroll_min_x = SDL_max(cursor_right, right_border) - right_border;
    const size_t scroll_min_y = SDL_max(cursor_bottom, bottom_border) - bottom_border;
    // largest offset before the cursor goes past the left/top margin
    const size_t scroll_max_x = SDL_max(on_screen.x, left_border) - left_border;
    const size_t scroll_max_y = SDL_max(on_screen.y, top_border) - top_border;

    file_view->scrolling.x = SDL_clamp(file_view->scrolling.x, scroll_min_x, scroll_max_x);
    file_view->scrolling.y = SDL_clamp(file_view->scrolling.y, scroll_min_y, scroll_max_y);
}

static void ui_sdl3_draw_panels(SDL3Ui* ui)
{
    const ColorTheme* theme = &ui->medit->config.color_theme;

    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_MENU_BAR)) {
        ui_sdl3_draw_panel(ui, ui->layout.menu_bar, theme->menu_bar_bg);
    }
    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_STATUS_BAR)) {
        ui_sdl3_draw_panel(ui, ui->layout.status_bar, theme->status_bar_bg);
        ui_sdl3_draw_status_bar_text(ui);
    }
    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_SIDE_PANEL)) {
        ui_sdl3_draw_panel(ui, ui->layout.side_panel, theme->sidebar_bg);
    }
    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_BOTTOM_PANEL)) {
        ui_sdl3_draw_panel(ui, ui->layout.bottom_panel, theme->bottom_panel_bg);
    }
}

static void ui_sdl3_flush_draw_list(SDL3Ui* ui, const UiDrawCmdList* list)
{
    for (size_t i = 0; i < list->count; i++) {
        const UiDrawCmd* cmd = &list->items[i];
        switch (cmd->kind) {
            case UI_CMD_RECT_FILLED: {
                SDL_FRect r = rect_to_sdl_frect(cmd->rect);
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->color));
                SDL_RenderFillRect(ui->renderer, &r);
            } break;
            case UI_CMD_RECT_OUTLINED: {
                SDL_FRect r = rect_to_sdl_frect(cmd->rect);
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->outline_color));
                SDL_RenderRect(ui->renderer, &r);
            } break;
            case UI_CMD_TEXT: {
                ui_sdl3_draw_text(
                    ui,
                    cmd->text,
                    strlen(cmd->text),
                    &ui->font_editor,
                    (PixelPos) { .x = (int)cmd->rect.x, .y = (int)cmd->rect.y },
                    cmd->color);
            } break;
            case UI_CMD_CLIP_PUSH: {
                SDL_Rect r = rect_to_sdl_rect(cmd->rect);
                SDL_SetRenderClipRect(ui->renderer, &r);
            } break;
            case UI_CMD_CLIP_POP: {
                SDL_SetRenderClipRect(ui->renderer, NULL);
            } break;
            case UI_CMD_SCROLLBAR: {
                float track_len = (float)cmd->rect.h;
                float thumb_h = cmd->thumb_ratio * track_len;
                SDL_FRect track = rect_to_sdl_frect(cmd->rect);
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->color));
                SDL_RenderFillRect(ui->renderer, &track);
                SDL_FRect thumb = {
                    .x = track.x,
                    .y = track.y + (cmd->scroll_pos * (track_len - thumb_h)),
                    .w = track.w,
                    .h = thumb_h,
                };
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->thumb_color));
                SDL_RenderFillRect(ui->renderer, &thumb);
            } break;
        }
    }
}

static void ui_sdl3_render_frame(SDL3Ui* ui)
{
    SDL_RenderPresent(ui->renderer);
}

static void ui_sdl3_compute_line_number_gutter_width(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    const int line_count = SDL_max(
        size_to_int(medit_file_view_file(medit, file_view)->lines.count),
        1000);
    if (line_count == ui->line_nr_cached_line_count) {
        return;
    }
    ui->line_nr_cached_line_count = line_count;
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

static void ui_sdl3_draw_line_number(SDL3Ui* ui, size_t row, Rect gutter, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    bool focused = medit_get_focused_file_view_group(medit) == group;

    const Color line_number_color = focused && row == file_view->cursors.items[0].line
        ? medit->config.color_theme.line_number_current
        : medit->config.color_theme.line_number;

    PixelPos pos = {
        .x = size_to_int(gutter.x),
        .y = size_to_int((row * ui->font_editor.line_spacing) + gutter.y)
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

    ui_sdl3_draw_text(ui, line_number, line_number_len, &ui->font_editor, pos, line_number_color);
}

static void ui_sdl3_draw_line(
    SDL3Ui* ui,
    size_t row,
    Line* line,
    Rect content,
    FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    PixelPos line_pos = {
        .x = size_to_int(content.x) - size_to_int(file_view->scrolling.x),
        .y = size_to_int((row * ui->font_editor.line_spacing) + content.y)
            - size_to_int(file_view->scrolling.y),
    };

    ui_sdl3_draw_text(
        ui,
        line->items,
        line->count,
        &ui->font_editor,
        line_pos,
        medit->config.color_theme.editor_fg);
}

typedef struct {
    // TODO when scrollable tab bars will be supported, there will be no real limit to the tab
    // content length
    char content[1024];
    size_t length;
    size_t width;
} FileViewTabText;

static void ui_sdl3_format_file_view_tab_text(
    SDL3Ui* ui,
    FileView* file_view,
    FileViewTabText* tab_text)
{
    // TODO draw a button instead of just an indicator
    static const char dirty_indicator[] = " ●";

    Meditor* medit = ui->medit;

    // TODO generate "Untitled" file names when creating the file
    const char* filename = medit_file_view_file(medit, file_view)->name
        ? medit_file_view_file(medit, file_view)->name
        : "Untitled";
    size_t filename_len = strlen(filename);

    const char* dirty_str = medit_file_view_file(medit, file_view)->dirty ? dirty_indicator : "";
    int written = snprintf(
        tab_text->content,
        sizeof(tab_text->content),
        " %.*s%s ",
        size_to_int(filename_len),
        filename,
        dirty_str);
    tab_text->length = int_to_size(written);

    int w = 0;
    TTF_MeasureString(ui->font_editor.main, tab_text->content, tab_text->length, 0, &w, NULL);
    tab_text->width = int_to_size(w);
}

static void ui_sdl3_draw_file_view_group_tab_bar(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    const LayoutSizes s = ui->layout.sizes;
    Rect tab_bar_area = group->area;
    Panel tab_bar = panel_cut_top(&tab_bar_area, s.tab_bar_height, s.separator_size);
    ui_sdl3_draw_panel(ui, tab_bar, medit->config.color_theme.tab_bar_bg);

    Panel tab = { .area = tab_bar.area };
    Rect remaining = tab_bar.area;
    for (size_t i = 0; i < group->count; ++i) {
        FileView* file_view = &group->items[i];
        FileViewTabText tab_text = { 0 };
        ui_sdl3_format_file_view_tab_text(ui, file_view, &tab_text);
        tab = panel_cut_left(&remaining, SDL_max(tab_text.width, 128), s.separator_size);
        const Color tab_color = i == group->displayed ? medit->config.color_theme.editor_bg
                                                      : medit->config.color_theme.tab_bar_bg;
        ui_sdl3_draw_panel(ui, tab, tab_color);

        int font_h = TTF_GetFontHeight(ui->font_editor.main);
        int tab_text_y = size_to_int(tab.area.y) + ((size_to_int(tab.area.h) - font_h) / 2);
        const char* tab_label = ui_sdl3_arena_str(ui, tab_text.content, tab_text.length);
        if (tab_label) {
            UiDrawCmd tab_text_cmd = {
                .kind = UI_CMD_TEXT,
                .rect = { .x = tab.area.x,
                          .y = int_to_size(tab_text_y),
                          .w = tab.area.w,
                          .h = tab.area.h },
                .color = ui->medit->config.color_theme.editor_fg,
                .text = tab_label,
            };
            dynarray_append(&ui->ui_draw_list_bg, tab_text_cmd);
        }
    }
}

// Queue background and tab bar draw commands for this group into the bg draw layer
// Does NOT draw file content lines — call ui_sdl3_draw_file_view_group_content for that
static void ui_sdl3_draw_file_view_group(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    ui_panel(&ui->ui_ctx_bg, group->area, medit->config.color_theme.editor_bg);

    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_TAB_BAR)) {
        ui_sdl3_draw_file_view_group_tab_bar(ui, group);
    }
}

// Draw file lines and line numbers directly to the renderer
// Must be called after the bg draw layer has been flushed
static void ui_sdl3_draw_file_view_group_content(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* displayed_file_view = medit_get_displayed_file_view_in_group(medit, group);
    Lines* lines = &medit_file_view_file(medit, displayed_file_view)->lines;

    const size_t first_rendered_line = displayed_file_view->scrolling.y
        / ui->font_editor.line_spacing;
    const size_t screen_lines = (int_to_size(ui->window_size.height) / ui->font_editor.line_spacing)
        + 1;
    const size_t rendered_line_count = SDL_min(lines->count, first_rendered_line + screen_lines);

    Rect area = group->content_area;
    Rect gutter = rect_cut_left(&area, int_to_size(ui->line_nr_padding));
    Rect content = area;

    const SDL_Rect gutter_clip = rect_to_sdl_rect(gutter);
    assert(SDL_SetRenderClipRect(ui->renderer, &gutter_clip));
    for (size_t row = first_rendered_line; row < rendered_line_count; ++row) {
        ui_sdl3_draw_line_number(ui, row, gutter, group);
    }

    const SDL_Rect content_clip = rect_to_sdl_rect(content);
    assert(SDL_SetRenderClipRect(ui->renderer, &content_clip));
    for (size_t row = first_rendered_line; row < rendered_line_count; ++row) {
        ui_sdl3_draw_line(ui, row, &lines->items[row], content, group);
    }

    assert(SDL_SetRenderClipRect(ui->renderer, NULL));
}

// TODO temporary function placing groups on screen till we have a proper layout engine
static void temp_ui_sdl3_update_file_view_groups_size(SDL3Ui* ui)
{
    ui->layout.sizes = UI_SDL3_DEFAULT_LAYOUT_SIZES;
    ui->layout.elements_shown = LAYOUT_MENU_BAR | LAYOUT_STATUS_BAR | LAYOUT_TAB_BAR
        | LAYOUT_SIDE_PANEL;
    ui_sdl3_recompute_layout(ui);
}

static void temp_ui_sdl3_setup_layout(SDL3Ui* ui)
{
    Meditor* medit = ui->medit;

    medit_load_file(medit, "./src/ui/sdl3/sdl3.c");

    // Create an empty file in some file view groups
    for (size_t i = 0; i < 2; ++i) {
        dynarray_append(&medit->file_views, (FileViewGroup) { 0 });
        medit->file_views.focused = medit->file_views.count - 1;
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
    }

    // Insert some text in the focused latest created group
    const char text[] = "😊😊😊😊😊😊ùùùù😊";
    medit_insert_text(medit, text, strlen(text));

    // Update the layout of the groups in a grid fashion
    temp_ui_sdl3_update_file_view_groups_size(ui);
}

static void report_perf_counter(PerfCounter* perf_counter, void* userdata)
{
    MEDIT_UNUSED(userdata);

    SDL_Log(
        "Active frames: %llu | Avg frame time: %.2fms",
        (unsigned long long)perf_counter->frame_count,
        SDL_NS_TO_MS((double)perf_counter->accumulated_ns) / (double)perf_counter->frame_count);
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
            Line* line = &medit_file_view_file(medit, file_view)->lines.items[0];

            UcGraphemeIter it = { 0 };
            uc_grapheme_iter_init(&it, (uint8_t*)line->items, line->count, cursor->byte);
            UcSpan out = { 0 };
            uc_grapheme_iter_next(&it, &out);
            cursor->len = out.len;
            printf("group %zu, fileview %zu, cursor->len=%zu\n", i, j, cursor->len);
        }
    }

    ui_sdl3_enable_cursor_blink(&ui);

    perf_counter_start_periodic_report(
        &ui.perf_counter,
        PERF_COUNTER_REPORT_PERIOD_MS,
        report_perf_counter,
        NULL);

    medit->running = true;
    while (medit->running) {
        bool should_render = ui_sdl3_handle_event(&ui);
        if (ui.editor_font_size != medit->config.editor_font_size) {
            ui_sdl3_unload_editor_font(&ui);
            ui_sdl3_load_editor_font(&ui);
            should_render = true;
        }

        if (!should_render) {
            continue;
        }

        ui_sdl3_clear(&ui);
        ui_sdl3_draw_frame_begin(&ui);
        {
            // 1. Queue background layer: panels, tab bars, group backgrounds
            ui_sdl3_draw_panels(&ui);
            ui_panel(&ui.ui_ctx_bg, ui.layout.editor_area, medit->config.color_theme.panel_border);

            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                ui_sdl3_compute_line_number_gutter_width(&ui, group);
                ui_sdl3_draw_file_view_group(&ui, group); // queues group bg + tab bar into bg layer
            }

            // 2. Flush background layer so file content is drawn on top
            ui_sdl3_flush_draw_list(&ui, &ui.ui_draw_list_bg);

            // 3. Draw file content directly (lines + line numbers)
            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                Rect text_area = group->content_area;
                rect_cut_left(&text_area, int_to_size(ui.line_nr_padding));

                ui_sdl3_scroll_file_view(&ui, text_area, group);
                ui_sdl3_draw_file_view_group_content(&ui, group);
                // 4. Queue cursor rects into overlay layer
                ui_sdl3_queue_cursor(&ui, text_area, group);
            }

            // 5. Flush overlay layer (cursor rects) on top of file content
            ui_sdl3_flush_draw_list(&ui, &ui.ui_draw_list_overlay);

            // 6. Draw cursor glyphs on top of cursor rects
            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                Rect text_area = group->content_area;
                rect_cut_left(&text_area, int_to_size(ui.line_nr_padding));
                ui_sdl3_draw_cursor_glyphs(&ui, text_area, group);
            }
        }
        ui_sdl3_render_frame(&ui);

        perf_counter_frame_end(&ui.perf_counter);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
