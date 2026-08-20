#pragma once

#include "clay_utils.h"
#include "theme.h"

#include <SDL3/SDL.h>

static inline Clay_Color to_clay_color(Color color)
{
    return (Clay_Color) { .r = color.r, .g = color.g, .b = color.b, .a = color.a };
}

typedef enum {
    TITLEBAR_BTN_NONE,
    TITLEBAR_BTN_MIN,
    TITLEBAR_BTN_MAX,
    TITLEBAR_BTN_CLOSE,
} TitlebarHoveredButton;

typedef struct {
    TitlebarHoveredButton hovered_button;
    SDL_Rect minimize_button_rect;
    SDL_Rect maximize_button_rect;
    SDL_Rect close_button_rect;
} TitlebarState;

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

void medit_ui_titlebar_init(Ui* ui, SDL_Window* window);

bool medit_ui_is_any_element_dragged(const Ui* ui);

void medit_ui_update_scroll_containers(Ui* ui);

void medit_ui_update_titlebar(Ui* ui, int window_width);

void medit_ui_layout_scrollbar(Ui* ui);

void medit_ui_layout_titlebar(Ui* ui);
