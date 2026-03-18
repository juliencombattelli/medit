#include "meditor.h"
#include "ui/sdl3/sdl3.h"

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
}
