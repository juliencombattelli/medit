#include "sdl3_internal.h"

#include "utils/utils.h"

#include <core/assert.h>
#include <core/utils.h>

static void ui_sdl3_resize_window_with_data(SDL3Ui* ui, PixelSize window_size)
{
    assert(window_size.width >= 0);
    assert(window_size.height >= 0);

    ui->window_size = window_size;
}

void ui_sdl3_resize_window(SDL3Ui* ui)
{
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(ui->window, &w, &h);

    ui_sdl3_resize_window_with_data(ui, (PixelSize) { .width = w, .height = h });
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

void ui_sdl3_enable_cursor_blink(SDL3Ui* ui)
{
    ui->cursor_blinker.show = false;
    ui->cursor_blinker.timer = SDL_AddTimer(0, ui_sdl3_on_cursor_should_blink, ui);
}

void ui_sdl3_disable_cursor_blink(SDL3Ui* ui)
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

static void ui_sdl3_dispatch_event(SDL3Ui* ui, SDL_Event* event)
{
    Meditor* medit = ui->medit;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            ui_sdl3_handle_save_of_dirty_files(ui);
            ui->medit->running = false;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            ui_sdl3_on_window_resized(ui, event->window.data1, event->window.data2);
            break;
        case SDL_EVENT_KEY_DOWN: {
            KeybindEvent keybind_event = ui_sdl3_keybind_translate_event(event);
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

bool ui_sdl3_handle_event(SDL3Ui* ui)
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

bool ui_sdl3_create(SDL3Ui* ui, Meditor* medit)
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

void ui_sdl3_destroy(SDL3Ui* ui)
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
