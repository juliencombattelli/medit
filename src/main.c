#include <core/meditor.h>
#include <core/utils.h>

#include <ui/sdl3/sdl3.h>

int main(int argc, char** argv)
{
    const ColorTheme default_color_theme = {
        .editor_fg = color_from_u32(0xD4D4D4FF),
        .editor_bg = color_from_u32(0x1F1F1FFF),
        .line_number = color_from_u32(0x6e7681FF),
        .line_number_current = color_from_u32(0xD4D4D4FF),
        .sidebar_bg = color_from_u32(0x181818FF),
        .cursor = color_from_u32(0xD4D4D4FF),
        .menu_bar_bg = color_from_u32(0x181818FF),
        .status_bar_bg = color_from_u32(0x181818FF),
        .tab_bar_bg = color_from_u32(0x181818FF),
        .tab_bar_bg_hovered = color_from_u32(0x1F1F1FFF),
        .tab_bar_bg_displayed = color_from_u32(
            0x00001FFF), // TODO dark blue while we don't have the accent bar
        .bottom_panel_bg = color_from_u32(0x181818FF),
        .panel_border = color_from_u32(0x2B2B2BFF),
        .scrollbar_thumb = color_from_u32(0xFFFFFF1F),
        .scrollbar_thumb_scroll_area_hovered = color_from_u32(0xFFFFFF3F),
        .scrollbar_thumb_hovered = color_from_u32(0xFFFFFF5F),
    };

    MEDIT_UNUSED(argc), MEDIT_UNUSED(argv);
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;
    medit.config.color_theme = default_color_theme;

    medit_ui_sdl3_run(&medit);
    medit_close_all_files(&medit);
}
