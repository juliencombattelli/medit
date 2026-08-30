#include "default_config.h"

#include <core/ui/color.h>

Config medit_default_config(void) {
    return (Config) {
        .editor_font_size = FONT_SIZE_DEFAULT,
        .editor_font_path = FONT_PATH_DEFAULT,
        .theme = {
            .color_scheme = {
                .editor_fg = color_from_u32(0xD4D4D4FFu),
                .editor_bg = color_from_u32(0x1F1F1FFFu),
                .line_number = color_from_u32(0x6e7681FFu),
                .line_number_current = color_from_u32(0xD4D4D4FFu),
                .sidebar_bg = color_from_u32(0x181818FFu),
                .cursor = color_from_u32(0xD4D4D4FFu),
                .menu_bar_bg = color_from_u32(0x181818FFu),
                .status_bar_bg = color_from_u32(0x181818FFu),
                .tab_bar_bg = color_from_u32(0x181818FFu),
                .tab_bar_bg_hovered = color_from_u32(0x1F1F1FFFu),
                // TODO dark blue while we don't have the accent bar
                .tab_bar_bg_displayed = color_from_u32(0x00001FFFu),
                .bottom_panel_bg = color_from_u32(0x181818FFu),
                .panel_border = color_from_u32(0x2B2B2BFFu),
                .scrollbar_thumb_inactive = color_from_u32(0xFFFFFF1Fu),
                .scrollbar_thumb_hovered = color_from_u32(0xFFFFFF3Fu),
                .scrollbar_thumb_active = color_from_u32(0xFFFFFF5Fu),
                .titlebar_ctrl_button_minimize = color_from_u32(0xFFFF00FF),
                .titlebar_ctrl_button_maximize = color_from_u32(0xFF00FFFF),
                .titlebar_ctrl_button_close    = color_from_u32(0x00FFFFFF),
            },
            .layout_settings = {
                .scrollbar_corner_radius = 6,
                .scrollbar_size = 12,
                .dragged_tab_transparency = 0x9F,
                .panels_gap = 8,
                .panels_corner_radius = {8, 8, 8, 8},
                .titlebar_height = 32,
                .titlebar_button_width = 46,
                .window_resize_border = 4,
            },
        },
    };
}
