#ifndef MEDIT_DISPLAY_SDL3_SDL3_INTERNAL_H_
#define MEDIT_DISPLAY_SDL3_SDL3_INTERNAL_H_

#include "utils/perf_counter.h"

#include <core/meditor.h>
#include <core/ui/ui.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdbool.h>

enum {
    TEXT_ARENA_SIZE = 4096,
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
    TTF_Text* text_cache[FONT_ID_COUNT];
    Font fonts[FONT_ID_COUNT];
    PixelSize window_size;
    CursorBlinker cursor_blinker;
    PerfCounter perf_counter;
    int line_nr_padding;
    int line_nr_max_digits;
    int line_nr_cached_line_count;
    int editor_font_size;
} SDL3Display;

// sdl3_renderer.c
const char* display_sdl3_arena_str(SDL3Display* display, const char* str, size_t len);
void display_sdl3_draw_frame_begin(SDL3Display* display);
void display_sdl3_clear(SDL3Display* display);
// void display_sdl3_draw_text(
//     SDL3Display* display,
//     const char* text,
//     size_t len,
//     Font* font,
//     PixelPos pos,
//     Color color);
void display_sdl3_render_frame(SDL3Display* display);

// sdl3_actions.c
extern const Actions DISPLAY_SDL3_ACTIONS;
void display_sdl3_handle_save_of_dirty_files(SDL3Display* display);
void display_sdl3_on_text_input(SDL3Display* display, const char* text);
void display_sdl3_on_key_down(SDL3Display* display, SDL_Event* event);

// sdl3_core.c
void display_sdl3_resize_window(SDL3Display* display);
bool display_sdl3_create(SDL3Display* display, Meditor* medit);
void display_sdl3_destroy(SDL3Display* display);
EventReaction display_sdl3_handle_event(SDL3Display* display);
void display_sdl3_enable_cursor_blink(SDL3Display* display);
void display_sdl3_disable_cursor_blink(SDL3Display* display);

// sdl3_font.c
void display_sdl3_ttf_setup(SDL3Display* display);
void display_sdl3_ttf_teardown(SDL3Display* display);
void display_sdl3_load_font(SDL3Display* display, FontId font_id);
void display_sdl3_unload_font(SDL3Display* display, FontId font_id);

// sdl3_file_view.c
void display_sdl3_queue_cursor(SDL3Display* display, Rect text_area, FileViewGroup* group);
void display_sdl3_draw_cursor_glyphs(SDL3Display* display, Rect text_area, FileViewGroup* group);
void display_sdl3_scroll_file_view(SDL3Display* display, Rect text_area, FileViewGroup* group);
void display_sdl3_compute_line_number_gutter_width(SDL3Display* display, FileViewGroup* group);
void display_sdl3_draw_file_view_group_content(SDL3Display* display, FileViewGroup* group);

// sdl3_keybind.c
KeybindEvent display_sdl3_keybind_translate_event(void* native_event);

#endif // MEDIT_DISPLAY_SDL3_SDL3_INTERNAL_H_
