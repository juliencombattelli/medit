#ifndef MEDIT_CORE_UI_H_
#define MEDIT_CORE_UI_H_

#include "color.h"
#include "rect.h"
#include <core/ui/theme.h>

#include "clay.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>


//------------------------------------------------------------------------------
// Mouse state handling (pointer, buttons & scroll wheels)
//------------------------------------------------------------------------------

// TODO consider keeping only pressed/released and transitioned_this_frame
typedef enum {
    // A mouse button is not currently down / was released at some point in the past
    MOUSE_BUTTON_RELEASED,
    // A mouse button click was released this frame
    MOUSE_BUTTON_RELEASED_THIS_FRAME,
    // A mouse button click occurred in a previous frame and is still currently held down this frame
    MOUSE_BUTTON_PRESSED,
    // A mouse button click occurred this frame
    MOUSE_BUTTON_PRESSED_THIS_FRAME,
} MouseButtonState;

#define MOUSE_BUTTON_LEFT   0u
#define MOUSE_BUTTON_MIDDLE 1u
#define MOUSE_BUTTON_RIGHT  2u
#define MOUSE_BUTTON_SIDE_1 3u
#define MOUSE_BUTTON_SIDE_2 4u
#define MOUSE_BUTTON_COUNT  5u

#define MOUSE_BUTTON_MASK(button) (1u << (unsigned)(button))

#define MOUSE_BUTTON_LEFT_MASK      MOUSE_BUTTON_MASK(MOUSE_BUTTON_LEFT)
#define MOUSE_BUTTON_MIDDLE_MASK    MOUSE_BUTTON_MASK(MOUSE_BUTTON_MIDDLE)
#define MOUSE_BUTTON_RIGHT_MASK     MOUSE_BUTTON_MASK(MOUSE_BUTTON_RIGHT)
#define MOUSE_BUTTON_SIDE_1_MASK    MOUSE_BUTTON_MASK(MOUSE_BUTTON_SIDE_1)
#define MOUSE_BUTTON_SIDE_2_MASK    MOUSE_BUTTON_MASK(MOUSE_BUTTON_SIDE_2)

typedef struct {
    MouseButtonState state;
    Clay_Vector2 click_origin;
} MouseButtonData;

typedef struct {
    Clay_Vector2 pos;
    Clay_Vector2 scroll_delta;
    MouseButtonData buttons[MOUSE_BUTTON_COUNT];
} MouseState;

void mouse_state_reset(MouseState* mouse_state);

void mouse_state_update(MouseState* mouse_state, uint32_t buttons);

//------------------------------------------------------------------------------
// Scrolling and dragging state handling
//------------------------------------------------------------------------------

typedef struct {
    float sensitivity_h;
    float sensitivity_v;
    bool use_both_wheels;
    bool enable_drag_on_edges;
} ScrollContainerConfig;

// State shared by all draggable elements in the UI as only one element can be dragged at a time
typedef struct {
    Clay_Vector2 click_origin;
    uint32_t active_id;
    bool is_droppable;
} DragState;

// Custom scroll container update function for containers having a scrollbar.
// Handle the following use-cases:
// - register the scrollbar as the currently dragged UI element in drag_state if it is being dragged
// - scroll when the scrollbar is moved
// - scroll when the pointer approaches the edges of the container and an element is being dragged
// - optionally use both horizontal/vertical mouse wheels when scrolling can only be done on one axe
void Clay_Ext_UpdateScrollContainerCustom(
    Clay_ElementId container_id,
    Clay_ElementId scrollbar_h_id,
    Clay_ElementId scrollbar_v_id,
    Clay_Vector2 mouse_pos,
    Clay_Vector2 delta,
    ScrollContainerConfig config,
    const MouseState* mouse_state,
    DragState* drag_state);

typedef enum {
    TITLEBAR_BTN_NONE,
    TITLEBAR_BTN_MIN,
    TITLEBAR_BTN_MAX,
    TITLEBAR_BTN_CLOSE,
} TitlebarHoveredButton;

typedef struct {
    TitlebarHoveredButton hovered_button;
    Rect minimize_button_rect;
    Rect maximize_button_rect;
    Rect close_button_rect;
} TitlebarState;

typedef struct {
    const char* container_id;
    ScrollContainerConfig config;
} ScrollContainerCustom;

typedef struct {
    Theme theme;
    TitlebarState titlebar_state;
    MouseState mouse_state;
    DragState drag_state;
    const ScrollContainerCustom* scroll_containers;
    size_t scroll_container_count;
    bool fullscreen;
    bool show_panel_left;
    bool show_panel_right;
    bool show_panel_bottom;
    bool show_status_bar;
} Ui;

bool medit_ui_is_any_element_dragged(const Ui* ui);

void medit_ui_update_scroll_containers(Ui* ui);

void medit_ui_update_titlebar(Ui* ui, int window_width);

void medit_ui_layout_scrollbar(Ui* ui);

void medit_ui_layout_titlebar(Ui* ui);

static inline Clay_Color to_clay_color(Color color)
{
    return (Clay_Color) { .r = color.r, .g = color.g, .b = color.b, .a = color.a };
}

#endif // MEDIT_CORE_UI_H_
