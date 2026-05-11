#include <core/meditor.h>
#include <core/utils.h>

#include <ui/sdl3/sdl3.h>

#include "default_settings.h"

int main(int argc, char** argv)
{
    MEDIT_UNUSED(argc), MEDIT_UNUSED(argv);
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;
    medit.config.color_theme = default_color_theme();

    medit_ui_sdl3_run(&medit);
    medit_close_all_files(&medit);
}
