#include "dynarray.h"
#include "meditor.h"
#include "platform/ansi-terminal/ansi_term.h"
#include "platform/sdl3/sdl3.h"

static const Color color_editor_fg = { .r = 0xD4, .g = 0xD4, .b = 0xD4, .a = 0xFF };
static const Color color_editor_bg = { .r = 0x1F, .g = 0x1F, .b = 0x1F, .a = 0xFF };
static const Color color_sidebar_bg = { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF };

int main(int argc, char** argv)
{
    (void)argc, (void)argv;

    Meditor medit = {
        .renderer = renderer_sdl3(),
        // .renderer = renderer_ansi_term(),
    };

    medit_renderer_create(&medit);

    medit.editor_font_size = FONT_SIZE_DEFAULT;
    medit.editor_font_path = FONT_PATH_DEFAULT;
    medit_load_font(&medit);

    meditor_new_file(&medit);

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

        medit_render_debug_grid(&medit);

        Lines* lines = &medit.focused_view.file->lines;
        int row = 0;
        dynarray_foreach(Line, line, lines)
        {
            if (line->count != 0) {
                medit_render_text(&medit, line->items, line->count, vec2(0, row), white);
            }
            row++;
        }

        medit_render_cursor(&medit, color_editor_fg);

        medit_renderer_present(&medit);
    }

    medit_unload_font(&medit);
    medit_renderer_destroy(&medit);
}
