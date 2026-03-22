#include "dynarray.h"
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

    // Create an empty file in a first file view group
    dynarray_append(&medit.file_views, (FileViewGroup) { 0 });
    medit.file_views.focused = medit.file_views.count - 1;
    medit_new_empty_file(&medit, &dynarray_last(&medit.file_views));

    // Create an empty file in a second file view group
    dynarray_append(&medit.file_views, (FileViewGroup) { 0 });
    medit.file_views.focused = medit.file_views.count - 1;
    medit_new_empty_file(&medit, &dynarray_last(&medit.file_views));

    const char text[] = "😊😊😊😊😊😊ùùùù😊";
    medit_insert_text(&medit, text, sizeof(text));
    medit_ui_sdl3_run(&medit);
    medit_close_files(&medit);
}
