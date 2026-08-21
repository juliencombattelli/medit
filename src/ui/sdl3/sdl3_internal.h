#ifndef MEDIT_UI_SDL3_SDL3_INTERNAL_H_
#define MEDIT_UI_SDL3_SDL3_INTERNAL_H_

#include "utils/perf_counter.h"

#include <core/meditor.h>
#include <core/ui/ui.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdbool.h>

enum {
    UI_TEXT_ARENA_SIZE = 4096,
};

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

typedef enum {
    REQUEST_HOT_RELOADING   = 1 << 0,
    REQUEST_RENDER          = 1 << 1,
} EventReaction;

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

typedef enum {
    FONT_ID_UI = 0,
    FONT_ID_EDITOR,
    FONT_ID_COUNT,
} FontId;

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
    Font fonts[FONT_ID_COUNT];
    PixelSize window_size;
    CursorBlinker cursor_blinker;
    PerfCounter perf_counter;
    int line_nr_padding;
    int line_nr_max_digits;
    int line_nr_cached_line_count;
    int editor_font_size;
} SDL3Ui;

// sdl3_renderer.c
const char* ui_sdl3_arena_str(SDL3Ui* ui, const char* str, size_t len);
void ui_sdl3_draw_frame_begin(SDL3Ui* ui);
void ui_sdl3_clear(SDL3Ui* ui);
// void ui_sdl3_draw_text(
//     SDL3Ui* ui,
//     const char* text,
//     size_t len,
//     Font* font,
//     PixelPos pos,
//     Color color);
void ui_sdl3_render_frame(SDL3Ui* ui);

// sdl3_actions.c
extern const Actions UI_SDL3_ACTIONS;
void ui_sdl3_handle_save_of_dirty_files(SDL3Ui* ui);
void ui_sdl3_on_text_input(SDL3Ui* ui, const char* text);
void ui_sdl3_on_key_down(SDL3Ui* ui, SDL_Event* event);

// sdl3_core.c
void ui_sdl3_resize_window(SDL3Ui* ui);
bool ui_sdl3_create(SDL3Ui* ui, Meditor* medit);
void ui_sdl3_destroy(SDL3Ui* ui);
EventReaction ui_sdl3_handle_event(SDL3Ui* ui);
void ui_sdl3_enable_cursor_blink(SDL3Ui* ui);
void ui_sdl3_disable_cursor_blink(SDL3Ui* ui);

// sdl3_font.c
void ui_sdl3_ttf_setup(SDL3Ui* ui);
void ui_sdl3_ttf_teardown(SDL3Ui* ui);
void ui_sdl3_load_font(SDL3Ui* ui, FontId font_id);
void ui_sdl3_unload_font(SDL3Ui* ui, FontId font_id);

// sdl3_file_view.c
void ui_sdl3_queue_cursor(SDL3Ui* ui, Rect text_area, FileViewGroup* group);
void ui_sdl3_draw_cursor_glyphs(SDL3Ui* ui, Rect text_area, FileViewGroup* group);
void ui_sdl3_scroll_file_view(SDL3Ui* ui, Rect text_area, FileViewGroup* group);
void ui_sdl3_compute_line_number_gutter_width(SDL3Ui* ui, FileViewGroup* group);
void ui_sdl3_draw_file_view_group_content(SDL3Ui* ui, FileViewGroup* group);

// sdl3_keybind.c
KeybindEvent ui_sdl3_keybind_translate_event(void* native_event);

#endif // MEDIT_UI_SDL3_SDL3_INTERNAL_H_
