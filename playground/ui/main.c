#include <ui/sdl3/sdl3_internal.h>

#include <core/assert.h>

#include "default_settings.h"

int main(void)
{
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;
    medit.config.color_theme = default_color_theme();

    SDL3Ui ui = { 0 };
    assert(ui_sdl3_create(&ui, &medit));

    ui_sdl3_load_editor_font(&ui);

    ui_sdl3_enable_cursor_blink(&ui);

    medit.running = true;
    while (medit.running) {
        bool should_render = ui_sdl3_handle_event(&ui);
        if (!should_render) {
            continue;
        }

        ui_sdl3_clear(&ui);
        ui_sdl3_draw_frame_begin(&ui);
        {
        }
        ui_sdl3_render_frame(&ui);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
