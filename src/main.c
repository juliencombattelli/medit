#include "keybind.h"
#include "keybind_sdl3.h"
#include "meditor.h"
#include "renderer_sdl3.h"

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

void action_toggle_debug_grid(Meditor* medit)
{
    medit->draw_debug_grid = !medit->draw_debug_grid;
}

void action_font_zoom_out(Meditor* medit)
{
    set_font_size_clamped(&medit->editor_font_size, medit->editor_font_size - 2);
    medit_unload_font(medit);
    medit_load_font(medit);
}

void action_font_zoom_in(Meditor* medit)
{
    set_font_size_clamped(&medit->editor_font_size, medit->editor_font_size + 2);
    medit_unload_font(medit);
    medit_load_font(medit);
}

void action_cursor_up(Meditor* medit)
{
    meditor_cursor_up(medit, 1);
}

void action_cursor_down(Meditor* medit)
{
    meditor_cursor_down(medit, 1);
}

void action_cursor_left(Meditor* medit)
{
    meditor_cursor_left(medit, 1);
}

void action_cursor_right(Meditor* medit)
{
    meditor_cursor_right(medit, 1);
}

void load_default_keymap(Keybind* keybind, Meditor* medit)
{
    keybind_bind(keybind, KEY_A, MOD_CTRL, action_toggle_debug_grid, medit);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, action_font_zoom_in, medit);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, action_font_zoom_in, medit);

    keybind_bind(keybind, KEY_NPAD_MINUS, MOD_CTRL, action_font_zoom_out, medit);
    keybind_bind(keybind, KEY_6, MOD_CTRL, action_font_zoom_out, medit);

    keybind_bind(keybind, KEY_UP, MOD_NONE, action_cursor_up, medit);
    keybind_bind(keybind, KEY_DOWN, MOD_NONE, action_cursor_down, medit);
    keybind_bind(keybind, KEY_LEFT, MOD_NONE, action_cursor_left, medit);
    keybind_bind(keybind, KEY_RIGHT, MOD_NONE, action_cursor_right, medit);
}

bool keycode_ctrl(SDL_Event event, SDL_Keycode keycode)
{
    return event.key.key == keycode && (event.key.mod & SDL_KMOD_CTRL);
}

static const Color color_editor_fg = { .r = 0xD4, .g = 0xD4, .b = 0xD4, .a = 0xFF };
static const Color color_editor_bg = { .r = 0x1F, .g = 0x1F, .b = 0x1F, .a = 0xFF };
static const Color color_sidebar_bg = { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF };

int main(int argc, char** argv)
{
    (void)argc, (void)argv;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    RendererSDL3 renderer_sdl3 = { 0 };

    SDL_Window* sdl_window = SDL_CreateWindow("Medit", 1280, 720, SDL_WINDOW_HIDDEN);

    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
    SDL_SetRenderVSync(sdl_renderer, 1);

    renderer_sdl3.window = sdl_window;
    renderer_sdl3.renderer = sdl_renderer;

    Meditor medit = {
        .renderer = create_sdl3_renderer(&renderer_sdl3),
    };

    medit.editor_font_size = FONT_SIZE_DEFAULT;
    medit.editor_font_path = FONT_PATH_DEFAULT;
    medit_load_font(&medit);

    SDL_ShowWindow(renderer_sdl3.window);

    SDL_StartTextInput(renderer_sdl3.window);

    SDL_GetWindowSize(
        renderer_sdl3.window,
        &renderer_sdl3.window_width,
        &renderer_sdl3.window_height);
    medit.grid_cols = renderer_sdl3.window_width / renderer_sdl3.cell_width;
    medit.grid_rows = renderer_sdl3.window_height / renderer_sdl3.cell_height;

    Keybind keybind = { 0 };

    printf("Loading keymapping\n");
    load_default_keymap(&keybind, &medit);

    // const char welcome_message[] = "😀 Hello, world! 😀";
    const char welcome_message[] = "Hello, world! 😀";
    {
        const int text_cells = medit_get_text_cells(&medit, welcome_message);
        meditor_append_text(&medit, welcome_message, text_cells);
    }

    bool running = true;
    bool input_in_frame = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event) != 0) {
            input_in_frame = true;
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_WINDOW_RESIZED:
                    renderer_sdl3.window_width = (int)event.window.data1;
                    renderer_sdl3.window_height = (int)event.window.data2;
                    medit.grid_cols = renderer_sdl3.window_width / renderer_sdl3.cell_width;
                    medit.grid_rows = renderer_sdl3.window_height / renderer_sdl3.cell_height;
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
                    const int text_cells = medit_get_text_cells(&medit, event.text.text);
                    meditor_append_text(&medit, event.text.text, text_cells);
                    meditor_cursor_right(&medit, text_cells);
                } break;
                case SDL_EVENT_KEYMAP_CHANGED: {
                    printf("Reloading keymapping\n");
                    keybind_reinit(&keybind);
                    load_default_keymap(&keybind, &medit);
                } break;
            }
        }

        if (!input_in_frame) {
            continue;
        }
        input_in_frame = false;

        medit_clear_screen(&medit, color_editor_bg);

        // Render text examples
        Color white = { 255, 255, 255, 255 };
        Color cyan = { 0, 255, 255, 255 };
        Color lime = { 0, 255, 0, 255 };

        medit_render_debug_grid(&medit);

        medit_render_text0(&medit, "TrueType Font Rendering", 10, 4, white);
        medit_render_text0(&medit, "With SDL_ttf", 10, 6, cyan);
        medit_render_text0(&medit, "Press ESC to exit", 10, 10, lime);

        if (medit.text_size != 0) {
            medit_render_text0(&medit, medit.text, 0, 0, color_editor_fg);
        }
        medit_render_cursor(&medit, color_editor_fg);

        medit_renderer_present(&medit);
    }

    medit_unload_font(&medit);
    medit_renderer_destroy(&medit);
}
