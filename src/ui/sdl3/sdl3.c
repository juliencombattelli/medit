#include "sdl3.h"
#include "assert.h"
#include "dynarray.h"
#include "font.h"
#include "keybind.h"
#include "meditor.h"
#include "perf_counter.h"
#include "safeint.h"
#include "sdl3_utils.h"
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
    int line_nr_padding;
    int line_nr_max_digits;
    int line_nr_cached_line_count;
    int editor_font_size;
} SDL3Ui;

static void temp_ui_sdl3_update_file_view_groups_size(SDL3Ui* ui);

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

static void set_font_size_clamped(int* font, int value)
{
    if (value > FONT_SIZE_MAX) {
        value = FONT_SIZE_MAX;
    }
    if (value < FONT_SIZE_MIN) {
        value = FONT_SIZE_MIN;
    }
    *font = value;
}

static void action_quit(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit->running = false;
}

static void action_font_zoom_out(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    set_font_size_clamped(&medit->config.editor_font_size, medit->config.editor_font_size - 2);
}

static void action_font_zoom_in(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    set_font_size_clamped(&medit->config.editor_font_size, medit->config.editor_font_size + 2);
}

static void action_font_zoom_default(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    set_font_size_clamped(&medit->config.editor_font_size, FONT_SIZE_DEFAULT);
}

static void action_cursor_up(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_up(medit);
}

static void action_cursor_down(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_down(medit);
}

static void action_cursor_left(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_left(medit);
}

static void action_cursor_right(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_right(medit);
}

static void action_restore_cursor(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    FileView* file_view = medit_get_focused_file_view(medit);
    file_view->cursors.count = 1;
    // TODO memset other cursors
}

static void action_cursor_line_begin(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_line_begin(medit);
}

static void action_cursor_line_end(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_line_end(medit);
}

static void action_cursor_file_begin(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_file_begin(medit);
}

static void action_cursor_file_end(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_file_end(medit);
}

static void action_add_cursor_down(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(medit);
    MEDIT_UNUSED(ui);
    // Vec2* prev_cursor = &medit->cursor_pos[medit->cursor_index];
    // Vec2* new_cursor = &medit->cursor_pos[++medit->cursor_index];
    // *new_cursor = vec2(prev_cursor->x, prev_cursor->y + 1);
}

static void action_dump_state(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    FileView* file_view = medit_get_focused_file_view(medit);

    printf("Dump state:\n");
    printf(
        "  cursor: byte=%zu, line=%zu; lines:%zu\n  lines:\n",
        file_view->cursors.items[0].byte,
        file_view->cursors.items[0].line,
        file_view->file->lines.count);
    Lines* lines = &file_view->file->lines;
    int row = 0;
    dynarray_foreach(Line, line, lines)
    {
        if (line->count != 0) {
            printf("    #%d:`%.*s`\n", row++, (int)line->count, line->items);
        } else {
            printf("    #%d:``\n", row++);
        }
    }
}

static void action_erase_line(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_erase_line(medit);
}

static void action_focus_file_view_group_left(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    size_t* focused = &medit->file_views.focused;
    if (*focused > 0) {
        *focused -= 1;
    }
}

static void action_focus_file_view_group_right(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    size_t* focused = &medit->file_views.focused;
    if (*focused < medit->file_views.count - 1) {
        *focused += 1;
    }
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

static void ui_sdl3_load_default_keybind(SDL3Ui* ui)
{
    Keybind* keybind = &ui->medit->keybind;

    keybind_bind(keybind, KEY_Q, MOD_CTRL, action_quit, ui->medit, ui);
    keybind_bind(keybind, KEY_S, MOD_CTRL, action_save_file, ui->medit, ui);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, action_font_zoom_in, ui->medit, ui);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, action_font_zoom_in, ui->medit, ui);
    keybind_bind(keybind, KEY_EQUALS, MOD_CTRL, action_font_zoom_default, ui->medit, ui);

    keybind_bind(keybind, KEY_NPAD_MINUS, MOD_CTRL, action_font_zoom_out, ui->medit, ui);
    keybind_bind(keybind, KEY_6, MOD_CTRL, action_font_zoom_out, ui->medit, ui);

    keybind_bind(keybind, KEY_UP, MOD_NONE, action_cursor_up, ui->medit, ui);
    keybind_bind(keybind, KEY_DOWN, MOD_NONE, action_cursor_down, ui->medit, ui);
    keybind_bind(keybind, KEY_LEFT, MOD_NONE, action_cursor_left, ui->medit, ui);
    keybind_bind(keybind, KEY_RIGHT, MOD_NONE, action_cursor_right, ui->medit, ui);
    keybind_bind(keybind, KEY_HOME, MOD_NONE, action_cursor_line_begin, ui->medit, ui);
    keybind_bind(keybind, KEY_END, MOD_NONE, action_cursor_line_end, ui->medit, ui);
    keybind_bind(keybind, KEY_HOME, MOD_CTRL, action_cursor_file_begin, ui->medit, ui);
    keybind_bind(keybind, KEY_END, MOD_CTRL, action_cursor_file_end, ui->medit, ui);

    keybind_bind(keybind, KEY_ESCAPE, MOD_NONE, action_restore_cursor, ui->medit, ui);
    keybind_bind(keybind, KEY_DOWN, MOD_CTRL_ALT, action_add_cursor_down, ui->medit, ui);

    keybind_bind(keybind, KEY_D, MOD_CTRL, action_dump_state, ui->medit, ui);

    keybind_bind(keybind, KEY_K, MOD_SHIFT_CTRL, action_erase_line, ui->medit, ui);

    keybind_bind(keybind, KEY_LEFT, MOD_ALT, action_focus_file_view_group_left, ui->medit, ui);
    keybind_bind(keybind, KEY_RIGHT, MOD_ALT, action_focus_file_view_group_right, ui->medit, ui);

    keybind_bind(keybind, KEY_O, MOD_CTRL, action_open_file_dialog, ui->medit, ui);
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

    ui_sdl3_load_default_keybind(ui);

    return true;
}

static void ui_sdl3_destroy(SDL3Ui* ui)
{
    SDL_StopTextInput(ui->window);

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

    temp_ui_sdl3_update_file_view_groups_size(ui);
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
    int message_len = snprintf(NULL, 0, fmt, file->name);
    char msg[message_len + 1];
    (void)snprintf(msg, sizeof msg, fmt, file->name);
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
            medit_get_focused_file_view(ui->medit)->file->dirty = true;
        } break;
        case SDLK_BACKSPACE:
            medit_erase_char(ui->medit);
            medit_get_focused_file_view(ui->medit)->file->dirty = true;
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
            ui_sdl3_load_default_keybind(ui);
        } break;
        default: break;
    }
}

static bool ui_sdl3_handle_event(SDL3Ui* ui)
{
    // Save current font size to monitor changes
    ui->editor_font_size = ui->medit->config.editor_font_size;

    // Block until an event arrives or a timeout, saving CPU.
    // Pass NULL to avoid consuming the first event, so PollEvent drains everything uniformly.
    if (SDL_WaitEventTimeout(NULL, WAIT_FOR_EVENT_TIMEOUT_MS)) {
        perf_counter_frame_begin(&ui->perf_counter);
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            ui_sdl3_reset_cursor_blinking_timer_on_input(ui, &event);
            ui_sdl3_dispatch_event(ui, &event);
        }
        return true;
    }
    // Timeout (no events): nothing changed, skip render.
    perf_counter_frame_discard(&ui->perf_counter);
    return false;
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

    TTF_Text* text_obj = ui->text_cache;

    TTF_SetTextFont(text_obj, font->main);
    TTF_SetTextString(text_obj, text, len);
    TTF_SetTextColor(text_obj, color_to_RGBA_args(color));

    TTF_DrawRendererText(text_obj, (float)pos.x, (float)pos.y + (float)font->line_centering_offset);
}

static Rect ui_sdl3_cursor_rect(
    SDL3Ui* ui,
    FileViewGroup* group,
    const Cursor* cursor,
    FileView* file_view)
{
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
        .x = group->area.x + int_to_size(line_w),
        .y = group->area.y + (cursor->line * ui->font_editor.line_spacing),
        .w = int_to_size(cursor_w),
        .h = ui->font_editor.line_spacing,
    };
}

static void ui_sdl3_draw_cursor(SDL3Ui* ui, FileViewGroup* group)
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
        const Rect on_screen = ui_sdl3_cursor_rect(ui, group, cursor, file_view);

        SDL_FRect cursor_frect = {
            .x = (float)(on_screen.x + int_to_size(ui->line_nr_padding) - file_view->scrolling.x),
            .y = (float)(on_screen.y - file_view->scrolling.y),
            .w = (float)on_screen.w,
            .h = (float)on_screen.h,
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
                .x = size_to_int(on_screen.x) + ui->line_nr_padding
                    - size_to_int(file_view->scrolling.x),
                .y = size_to_int(on_screen.y - file_view->scrolling.y),
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
    const Rect on_screen = ui_sdl3_cursor_rect(ui, group, cursor, file_view);

    const size_t margin_x = ui->font_editor.default_cursor_width * 3;
    const size_t margin_y = ui->font_editor.line_spacing * 3;

    const size_t right_border = group->area.x + group->area.w - margin_x
        - int_to_size(ui->line_nr_padding);
    const size_t bottom_border = group->area.y + group->area.h - margin_y;
    const size_t left_border = group->area.x + margin_x;
    const size_t top_border = group->area.y + margin_y;

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

static void ui_sdl3_render(SDL3Ui* ui)
{
    SDL_RenderPresent(ui->renderer);
}

static void ui_sdl3_compute_line_number_gutter_width(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    const int line_count = SDL_max(size_to_int(file_view->file->lines.count), 1000);
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

static void ui_sdl3_draw_line_number(SDL3Ui* ui, size_t row, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    bool focused = medit_get_focused_file_view_group(medit) == group;

    const Color line_number_color = focused && row == file_view->cursors.items[0].line
        ? medit->config.color_theme.line_number_current
        : medit->config.color_theme.line_number;

    PixelPos pos = {
        .x = size_to_int(group->area.x),
        .y = size_to_int((row * ui->font_editor.line_spacing) + group->area.y)
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

static void ui_sdl3_draw_line(SDL3Ui* ui, size_t row, Line* line, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    PixelPos line_pos = {
        .x = size_to_int(group->area.x) + ui->line_nr_padding - size_to_int(file_view->scrolling.x),
        .y = size_to_int((row * ui->font_editor.line_spacing) + group->area.y)
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

static void ui_sdl3_draw_file_view_group(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    Lines* lines = &file_view->file->lines;

    const size_t first_rendered_line = file_view->scrolling.y / ui->font_editor.line_spacing;
    const size_t screen_lines = (int_to_size(ui->window_size.height) / ui->font_editor.line_spacing)
        + 1;
    const size_t rendered_line_count = SDL_min(lines->count, first_rendered_line + screen_lines);

    const SDL_Rect gutter_clip = {
        .x = size_to_int(group->area.x),
        .y = size_to_int(group->area.y),
        .w = ui->line_nr_padding,
        .h = size_to_int(group->area.h),
    };
    assert(SDL_SetRenderClipRect(ui->renderer, &gutter_clip));
    for (size_t row = first_rendered_line; row < rendered_line_count; ++row) {
        ui_sdl3_draw_line_number(ui, row, group);
    }

    const SDL_Rect content_clip = {
        .x = size_to_int(group->area.x) + ui->line_nr_padding,
        .y = size_to_int(group->area.y),
        .w = size_to_int(group->area.w) - ui->line_nr_padding,
        .h = size_to_int(group->area.h),
    };
    assert(SDL_SetRenderClipRect(ui->renderer, &content_clip));
    for (size_t row = first_rendered_line; row < rendered_line_count; ++row) {
        ui_sdl3_draw_line(ui, row, &lines->items[row], group);
    }

    assert(SDL_SetRenderClipRect(ui->renderer, NULL));
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
    // for (size_t i = 0; i < 9; ++i) {
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

void report_perf_counter(PerfCounter* perf_counter, void* userdata)
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
            Line* line = &file_view->file->lines.items[0];

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

        for (size_t i = 0; i < medit->file_views.count; ++i) {
            FileViewGroup* group = &medit->file_views.items[i];
            { // TODO consider doing this on event instead of every frame
                ui_sdl3_compute_line_number_gutter_width(&ui, group);
                ui_sdl3_scroll_file_view(&ui, group);
            }

            ui_sdl3_draw_file_view_group_separator(&ui, group);
            ui_sdl3_draw_file_view_group(&ui, group);
            ui_sdl3_draw_cursor(&ui, group);
        }
        ui_sdl3_render(&ui);

        perf_counter_frame_end(&ui.perf_counter);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
