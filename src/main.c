#include "meditor.h"
#include "platform/sdl3/sdl3.h"

static const Color color_editor_fg = { .r = 0xD4, .g = 0xD4, .b = 0xD4, .a = 0xFF };
static const Color color_editor_bg = { .r = 0x1F, .g = 0x1F, .b = 0x1F, .a = 0xFF };
static const Color color_sidebar_bg = { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF };

int main(int argc, char** argv)
{
    (void)argc, (void)argv;

    Meditor medit = {
        .renderer = renderer_sdl3(),
    };

    medit_renderer_create(&medit);

    medit.editor_font_size = FONT_SIZE_DEFAULT;
    medit.editor_font_path = FONT_PATH_DEFAULT;
    medit_load_font(&medit);

    // const char welcome_message[] = "😀 Hello, world! 😀";
    const char welcome_message[] = "Hello, world! 😀";
    {
        const int text_cells = medit_get_text_cells(&medit, welcome_message);
        meditor_append_text(&medit, welcome_message, text_cells);
    }

    medit.running = true;
    medit.input_in_frame = true;
    while (medit.running) {
        medit_handle_events(&medit);

        if (!medit.input_in_frame) {
            continue;
        }
        medit.input_in_frame = false;

        medit_clear_screen(&medit, color_editor_bg);

        // Render text examples
        Color white = { 255, 255, 255, 255 };
        Color cyan = { 0, 255, 255, 255 };
        Color lime = { 0, 255, 0, 255 };

        medit_render_debug_grid(&medit);

        medit_render_text0(&medit, "TrueType Font Rendering", vec2(10, 4), white);
        medit_render_text0(&medit, "Using renderer ", vec2(10, 6), cyan);
        medit_render_text0(&medit, medit.renderer.name, vec2(25, 6), cyan);
        medit_render_text0(&medit, "Press ESC to exit", vec2(10, 10), lime);

        if (medit.text_size != 0) {
            medit_render_text0(&medit, medit.text, vec2(0, 0), color_editor_fg);
        }
        medit_render_cursor(&medit, color_editor_fg);

        medit_renderer_present(&medit);
    }

    medit_unload_font(&medit);
    medit_renderer_destroy(&medit);
}
