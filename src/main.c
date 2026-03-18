#include "dynarray.h"
#include "meditor.h"
#include "platform/ansi-terminal/ansi_term.h"
#include "platform/sdl3/sdl3.h"

#include "ui/sdl3/sdl3.h"

static const Color color_editor_fg = { .r = 0xD4, .g = 0xD4, .b = 0xD4, .a = 0xFF };
static const Color color_editor_bg = { .r = 0x1F, .g = 0x1F, .b = 0x1F, .a = 0xFF };
static const Color color_sidebar_bg = { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF };

#define medit_gui_create
#define medit_gui_destroy
#define medit_gui_load_font
#define medit_gui_unload_font
#define medit_gui_handle_event
#define medit_gui_clear
#define medit_gui_draw_debug_grid
#define medit_gui_draw_text
#define medit_gui_draw_cursor
#define medit_gui_render

#define HEX2RGBA()

int main(int argc, char** argv)
{
    (void)argc, (void)argv;
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;

    medit.config.color_theme.editor_fg = color_from_u32(0xD4D4D4FF);
    medit.config.color_theme.editor_bg = color_from_u32(0x1F1F1FFF);
    medit.config.color_theme.sidebar_bg = color_from_u32(0x181818FF);
    medit.config.color_theme.cursor = color_from_u32(0xD4D4D4FF);

    meditor_new_empty_file(&medit);
    medit_ui_sdl3_run(&medit);
    meditor_close_files(&medit);

    // Meditor medit = {
    //     .renderer = renderer_sdl3(),
    //     // .renderer = renderer_ansi_term(),
    // };

    // medit_renderer_create(&medit);

    // medit.startup_config.editor_font_size = FONT_SIZE_DEFAULT;
    // medit.startup_config.editor_font_path = FONT_PATH_DEFAULT;
    // medit_load_font(&medit);

    // meditor_new_empty_file(&medit);

    // medit.running = true;
    // medit.input_in_frame = true;
    // while (medit.running) {
    //     medit_handle_events(&medit);

    //     if (!medit.input_in_frame) {
    //         continue;
    //     }
    //     medit.input_in_frame = false;

    //     medit_clear_screen(&medit, color_editor_bg);

    //     // Render text examples
    //     Color white = { 255, 255, 255, 255 };

    //     medit_render_debug_grid(&medit);

    //     Lines* lines = &medit.focused_view.file->lines;
    //     size_t row = 0;
    //     dynarray_foreach(Line, line, lines)
    //     {
    //         if (line->items != NULL) {
    //             medit_render_text(
    //                 &medit,
    //                 line->items,
    //                 line->count,
    //                 (Cell) { .col = 0, .row = row },
    //                 white);
    //         }
    //         row++;
    //     }

    //     medit_render_cursor(&medit, color_editor_fg);

    //     medit_renderer_present(&medit);
    // }

    // meditor_close_files(&medit);

    // medit_unload_font(&medit);
    // medit_renderer_destroy(&medit);
}
