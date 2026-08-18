#pragma once

#include "clay_utils.h"
#include "theme.h"
#include "titlebar.h"

static inline Clay_Color to_clay_color(Color color) {
    return (Clay_Color) { .r = color.r, .g = color.g, .b = color.b, .a = color.a };
}

typedef struct {
    const char* container_id;
    ScrollContainerConfig config;
} ScrollContainerCustom;

typedef struct {
    UiTheme theme;
    TitlebarState titlebar_state;
    MouseState mouse_state;
    DragState drag_state;
    const ScrollContainerCustom* scroll_containers;
    size_t scroll_container_count;
} Ui;

bool medit_ui_is_any_element_dragged(const Ui* ui);

void medit_ui_update_scroll_containers(Ui* ui);

// Lay out a scrollbar with predefined settings for the opened element
void medit_ui_layout_scrollbar(const Ui* ui);
