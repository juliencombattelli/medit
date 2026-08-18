#pragma once

#include "../../src/core/color.h"

#include "clay.h"

typedef struct {
    Color editor_fg;
    Color editor_bg;
    Color line_number;
    Color line_number_current;
    Color sidebar_bg;
    Color cursor;
    Color menu_bar_bg;
    Color status_bar_bg;
    Color tab_bar_bg;
    Color tab_bar_bg_hovered;
    Color tab_bar_bg_displayed;
    Color bottom_panel_bg;
    Color panel_border;
    Color scrollbar_thumb_inactive;
    Color scrollbar_thumb_hovered;
    Color scrollbar_thumb_active;
} ColorTheme2;

typedef struct {
    ColorTheme2 colors;
    uint8_t dragged_tab_transparency;
    uint32_t titlebar_height;
    uint32_t titlebar_button_width;
    uint32_t window_resize_border;
    uint32_t scrollbar_size;
    uint32_t scrollbar_corner_radius;
    uint16_t panels_gap;
    Clay_CornerRadius panels_corner_radius;
} UiTheme;
