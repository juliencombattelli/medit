#include "keybind.h"
#include "keybind_sdl3.h"
#include "meditor.h"
#include "renderer.h"

enum {
    FONT_SIZE_MIN = 2,
    FONT_SIZE_MAX = 128,
    FONT_SIZE_DEFAULT = 20,
};

#define FONT_PATH_DEFAULT "asset/font/consola.ttf"

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

typedef struct {
    Meditor* medit;
    RendererSDL* renderer;
} BundledMeditorData;

void action_toggle_debug_grid(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;
    data->medit->draw_debug_grid = !data->medit->draw_debug_grid;
}

void action_font_zoom_out(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;

    set_font_size_clamped(&data->medit->editor_font_size, data->medit->editor_font_size - 2);
    sdl_render_unload_font(data->renderer, data->medit);
    sdl_render_load_font(data->renderer, data->medit);
}

void action_font_zoom_in(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;

    set_font_size_clamped(&data->medit->editor_font_size, data->medit->editor_font_size + 2);
    sdl_render_unload_font(data->renderer, data->medit);
    sdl_render_load_font(data->renderer, data->medit);
}

void action_cursor_up(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;
    meditor_cursor_up(data->medit, 1);
}

void action_cursor_down(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;
    meditor_cursor_down(data->medit, 1);
}

void action_cursor_left(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;
    meditor_cursor_left(data->medit, 1);
}

void action_cursor_right(void* user_data)
{
    BundledMeditorData* data = (BundledMeditorData*)user_data;
    meditor_cursor_right(data->medit, 1);
}

void load_default_keymap(Keybind* keybind, BundledMeditorData* data)
{
    keybind_bind(keybind, KEY_A, MOD_CTRL, action_toggle_debug_grid, data);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, action_font_zoom_in, data);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, action_font_zoom_in, data);

    keybind_bind(keybind, KEY_NPAD_MINUS, MOD_CTRL, action_font_zoom_out, data);
    keybind_bind(keybind, KEY_6, MOD_CTRL, action_font_zoom_out, data);

    keybind_bind(keybind, KEY_UP, MOD_NONE, action_cursor_up, data);
    keybind_bind(keybind, KEY_DOWN, MOD_NONE, action_cursor_down, data);
    keybind_bind(keybind, KEY_LEFT, MOD_NONE, action_cursor_left, data);
    keybind_bind(keybind, KEY_RIGHT, MOD_NONE, action_cursor_right, data);
}

bool keycode_ctrl(SDL_Event event, SDL_Keycode keycode)
{
    return event.key.key == keycode && (event.key.mod & SDL_KMOD_CTRL);
}

int main(int argc, char** argv)
{
    (void)argc, (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    Meditor medit = { 0 };
    RendererSDL renderer = { 0 };
    {
        SDL_Window* sdl_window = SDL_CreateWindow("Medit", 1280, 720, SDL_WINDOW_HIDDEN);

        SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
        SDL_SetRenderVSync(sdl_renderer, 1);

        renderer.window = sdl_window;
        renderer.renderer = sdl_renderer;
    }

    medit.editor_font_size = FONT_SIZE_DEFAULT;
    medit.editor_font_path = FONT_PATH_DEFAULT;
    sdl_render_load_font(&renderer, &medit);

    bool running = true;
    SDL_ShowWindow(renderer.window);

    SDL_StartTextInput(renderer.window);

    SDL_GetWindowSize(renderer.window, &renderer.window_width, &renderer.window_height);
    medit.grid_cols = renderer.window_width / renderer.cell_width;
    medit.grid_rows = renderer.window_height / renderer.cell_height;

    Keybind keybind = { 0 };

    BundledMeditorData medit_bundle = {
        .medit = &medit,
        .renderer = &renderer,
    };

    printf("Loading keymapping\n");
    load_default_keymap(&keybind, &medit_bundle);

    // const char welcome_message[] = "😀 Hello, world! 😀";
    const char welcome_message[] = "Hello, world! 😀";
    {
        const int text_cells = sdl_get_text_cells(&renderer, welcome_message);
        meditor_append_text(&medit, welcome_message, text_cells);
    }

    bool input_in_frame = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event) != 0) {
            input_in_frame = true;
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_WINDOW_RESIZED:
                    renderer.window_width = (int)event.window.data1;
                    renderer.window_height = (int)event.window.data2;
                    medit.grid_cols = renderer.window_width / renderer.cell_width;
                    medit.grid_rows = renderer.window_height / renderer.cell_height;
                    break;
                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    KeybindEvent keybind_event = keybind_sdl3_translate_event(&event);
                    keybind_handle_event(&keybind, &keybind_event);
                    break;
                }
                case SDL_EVENT_TEXT_INPUT: {
                    const int text_cells = sdl_get_text_cells(&renderer, event.text.text);
                    meditor_append_text(&medit, event.text.text, text_cells);
                    meditor_cursor_right(&medit, text_cells);
                } break;
                case SDL_EVENT_KEYMAP_CHANGED: {
                    printf("Reloading keymapping\n");
                    keybind_reinit(&keybind);
                    load_default_keymap(&keybind, &medit_bundle);
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

        sdl_render_text0(&renderer, &medit, "TrueType Font Rendering", 10, 4, white);
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
