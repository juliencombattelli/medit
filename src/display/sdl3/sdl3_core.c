#include "sdl3_internal.h"

#include "utils/utils.h"

#include <core/assert.h>
#include <core/utils.h>

static void display_sdl3_resize_window_with_data(SDL3Display* display, PixelSize window_size)
{
    assert(window_size.width >= 0);
    assert(window_size.height >= 0);

    display->window_size = window_size;
}

void display_sdl3_resize_window(SDL3Display* display)
{
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(display->window, &w, &h);

    display_sdl3_resize_window_with_data(display, (PixelSize) { .width = w, .height = h });
}

static void display_sdl3_on_window_resized(SDL3Display* display, int w, int h)
{
    display_sdl3_resize_window_with_data(
        display,
        (PixelSize) {
            .width = w,
            .height = h,
        });
}

static Uint32 display_sdl3_on_cursor_should_blink(void* userdata, SDL_TimerID timer_id, Uint32 interval)
{
    MEDIT_UNUSED(timer_id);
    MEDIT_UNUSED(interval);

    SDL3Display* display = userdata;
    display->cursor_blinker.show = !display->cursor_blinker.show;

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

void display_sdl3_enable_cursor_blink(SDL3Display* display)
{
    display->cursor_blinker.show = false;
    display->cursor_blinker.timer = SDL_AddTimer(0, display_sdl3_on_cursor_should_blink, display);
}

void display_sdl3_disable_cursor_blink(SDL3Display* display)
{
    SDL_RemoveTimer(display->cursor_blinker.timer);
}

static void display_sdl3_reset_cursor_blinking_timer_on_input(SDL3Display* display, SDL_Event* event)
{
    switch (event->type) {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_KEY_DOWN: {
            display_sdl3_disable_cursor_blink(display);
            display_sdl3_enable_cursor_blink(display);
        } break;
        default: break;
    }
}

static void display_sdl3_dispatch_event(SDL3Display* display, SDL_Event* event)
{
    Meditor* medit = display->medit;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            display_sdl3_handle_save_of_dirty_files(display);
            display->medit->running = false;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            display_sdl3_on_window_resized(display, event->window.data1, event->window.data2);
            break;
        case SDL_EVENT_KEY_DOWN: {
            KeybindEvent keybind_event = display_sdl3_keybind_translate_event(event);
            if (keybind_handle_event(&medit->keybind, &keybind_event)) {
                break;
            }
            display_sdl3_on_key_down(display, event);
        } break;
        case SDL_EVENT_TEXT_INPUT: {
            display_sdl3_on_text_input(display, event->text.text);
        } break;
        case SDL_EVENT_KEYMAP_CHANGED: {
            printf("Reloading keymapping\n");
            keybind_reinit(&medit->keybind);
            medit_load_default_keybind_full(display->medit, &DISPLAY_SDL3_ACTIONS, display);
        } break;
        case SDL_EVENT_MOUSE_WHEEL: {
        } break;
        default: break;
    }
}

EventReaction display_sdl3_handle_event(SDL3Display* display)
{
    // Save current font size to monitor changes
    display->editor_font_size = display->medit->config.editor_font_size;

    // Block until an event arrives or a timeout, saving CPU
    // Pass NULL to avoid consuming the first event, so PollEvent drains everything uniformly
    if (SDL_WaitEventTimeout(NULL, WAIT_FOR_EVENT_TIMEOUT_MS)) {
        perf_counter_frame_begin(&display->perf_counter);
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
#ifdef MEDIT_HOT_RELOAD_ENABLED
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F5) {
                return REQUEST_HOT_RELOADING;
            }
#endif
            display_sdl3_reset_cursor_blinking_timer_on_input(display, &event);
            display_sdl3_dispatch_event(display, &event);
        }
        return REQUEST_RENDER;
    }
    // Timeout (no events): nothing changed, skip render
    perf_counter_frame_discard(&display->perf_counter);
    return 0;
}

bool display_sdl3_create(SDL3Display* display, Meditor* medit)
{
    // NOTE: SDL_ttf is intentionally NOT initialized here. Its whole lifecycle
    // (TTF_Init/TTF_Quit, text engine, fonts, text cache) is owned per run cycle
    // by display_sdl3_ttf_setup/display_sdl3_ttf_teardown so that it is fully recreated
    // across every hot reload. See display_sdl3_ttf_setup for the rationale.
    try(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = SDL_CreateWindow(
        "Medit",
        DEFAULT_WINDOW_WIDTH,
        DEFAULT_WINDOW_HEIGHT,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);
    try(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    try(renderer);

    printf("[DEBUG] Selected renderer: %s\n", SDL_GetRendererName(renderer));

    // try(SDL_SetRenderVSync(renderer, 1));
    try(SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED));

    display->medit = medit;
    display->window = window;
    display->renderer = renderer;

    try(SDL_ShowWindow(display->window));

    try(SDL_StartTextInput(display->window));

    medit_load_default_keybind_full(medit, &DISPLAY_SDL3_ACTIONS, display);

    return true;
}

// The SDL_ttf subsystem (text engine, fonts, text cache, TTF_Quit) is torn
// down separately by display_sdl3_ttf_teardown before this point.
void display_sdl3_destroy(SDL3Display* display)
{
    SDL_StopTextInput(display->window);

    SDL_DestroyRenderer(display->renderer);
    SDL_DestroyWindow(display->window);

    SDL_Quit();

    *display = (SDL3Display) { 0 };
}
