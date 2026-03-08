#include "meditor.h"
#include "renderer.h"

#include <assert.h>

bool keycode_ctrl(SDL_Event event, SDL_Keycode keycode)
{
    return event.key.key == keycode && (event.key.mod & SDL_KMOD_CTRL);
}

#define FONT_SIZE_MIN 2
#define FONT_SIZE_MAX 128

#define DEFAULT_FONT_SIZE 20
#define DEFAULT_FONT_PATH "asset/font/Consola.ttf"

void set_font_size_clamped(int* font, int value)
{
    if (value > FONT_SIZE_MAX) {
        value = FONT_SIZE_MAX;
    }
    if (value < FONT_SIZE_MIN) {
        value = FONT_SIZE_MIN;
    }
    *font = value;
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    Meditor medit = {};
    RendererSDL renderer = {};
    {
        SDL_Window* sdl_window = SDL_CreateWindow(
            "Medit",
            1280,
            720,
            SDL_WINDOW_HIDDEN);

        SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
        SDL_SetRenderVSync(sdl_renderer, 1);

        renderer.window = sdl_window;
        renderer.renderer = sdl_renderer;
    }

    int font_size = DEFAULT_FONT_SIZE;
    const char* font_path = DEFAULT_FONT_PATH;
    sdl_render_load_font(&renderer, &medit, font_path, font_size);

    bool running = true;
    SDL_ShowWindow(renderer.window);

    SDL_StartTextInput(renderer.window);

    SDL_GetWindowSize(
        renderer.window,
        &renderer.window_width,
        &renderer.window_height);
    medit.grid_cols = renderer.window_width / renderer.cell_width;
    medit.grid_rows = renderer.window_height / renderer.cell_height;

    // const char welcome_message[] = "😀 Hello, world! 😀";
    const char welcome_message[] = "Hello, world! 😀";
    const int text_cells = sdl_get_text_cells(&renderer, welcome_message);
    meditor_append_text(&medit, welcome_message, text_cells);

    bool input_in_frame = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event) != 0) {
            input_in_frame = true;
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    renderer.window_width = (int)event.window.data1;
                    renderer.window_height = (int)event.window.data2;
                    medit.grid_cols = renderer.window_width
                        / renderer.cell_width;
                    medit.grid_rows = renderer.window_height
                        / renderer.cell_height;
                    break;
                case SDL_EVENT_KEY_DOWN: {
                    if (keycode_ctrl(event, SDLK_G)) {
                        medit.draw_debug_grid = !medit.draw_debug_grid;
                    }

                    SDL_Keycode keycode = SDL_GetKeyFromScancode(
                        event.key.scancode,
                        event.key.mod,
                        false);

                    if (keycode_ctrl(event, SDLK_KP_MINUS)
                        || (keycode == '-'
                            && (event.key.mod & SDL_KMOD_CTRL))) {
                        set_font_size_clamped(&font_size, font_size - 2);
                        sdl_render_unload_font(&renderer, &medit);
                        sdl_render_load_font(
                            &renderer,
                            &medit,
                            font_path,
                            font_size);
                    } else if (
                        keycode_ctrl(event, SDLK_KP_PLUS)
                        || (keycode == '+'
                            && (event.key.mod & SDL_KMOD_CTRL))) {
                        set_font_size_clamped(&font_size, font_size + 2);
                        sdl_render_unload_font(&renderer, &medit);
                        sdl_render_load_font(
                            &renderer,
                            &medit,
                            font_path,
                            font_size);
                    }
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    if (event.key.key == SDLK_UP) {
                        meditor_cursor_up(&medit, 1);
                    }
                    if (event.key.key == SDLK_DOWN) {
                        meditor_cursor_down(&medit, 1);
                    }
                    if (event.key.key == SDLK_LEFT) {
                        meditor_cursor_left(&medit, 1);
                    }
                    if (event.key.key == SDLK_RIGHT) {
                        meditor_cursor_right(&medit, 1);
                    }
                    break;
                }
                case SDL_EVENT_TEXT_INPUT: {
                    const int text_cells = sdl_get_text_cells(
                        &renderer,
                        event.text.text);
                    meditor_append_text(&medit, event.text.text, text_cells);
                    meditor_cursor_right(&medit, text_cells);
                } break;
            }
        }

        if (!input_in_frame) {
            continue;
        }
        input_in_frame = false;

        SDL_SetRenderDrawColor(renderer.renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer.renderer);

        // Render text examples
        Color white = { 255, 255, 255, 255 };
        Color cyan = { 0, 255, 255, 255 };
        Color lime = { 0, 255, 0, 255 };

        sdl_render_debug_grid(&renderer, &medit);

        sdl_render_text0(
            &renderer,
            &medit,
            "TrueType Font Rendering",
            10,
            4,
            white);
        sdl_render_text0(&renderer, &medit, "With SDL_ttf", 10, 6, cyan);
        sdl_render_text0(&renderer, &medit, "Press ESC to exit", 10, 10, lime);

        if (medit.text_size != 0) {
            sdl_render_text0(&renderer, &medit, medit.text, 0, 0, white);
        }
        sdl_render_cursor(&renderer, &medit, white);

        SDL_RenderPresent(renderer.renderer);
    }

    SDL_StopTextInput(renderer.window);

    TTF_CloseFont(renderer.font_editor);
    SDL_DestroyRenderer(renderer.renderer);
    SDL_DestroyWindow(renderer.window);
    TTF_Quit();
    SDL_Quit();
}
