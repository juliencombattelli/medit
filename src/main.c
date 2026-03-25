#include "meditor.h"
#include "ui/sdl3/sdl3.h"
#include "utils.h"

int main(int argc, char** argv)
{
    const ColorTheme default_color_theme = {
        .editor_fg = color_from_u32(0xD4D4D4FF),
        .editor_bg = color_from_u32(0x1F1F1FFF),
        .line_number = color_from_u32(0x6e7681FF),
        .line_number_current = color_from_u32(0xD4D4D4FF),
        .sidebar_bg = color_from_u32(0x181818FF),
        .cursor = color_from_u32(0xD4D4D4FF),
    };

    MEDIT_UNUSED(argc), MEDIT_UNUSED(argv);
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;
    medit.config.color_theme = default_color_theme;

    medit_ui_sdl3_run(&medit);
    medit_close_files(&medit);
}
