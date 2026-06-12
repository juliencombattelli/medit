#include "clay_utils.h"

#include <core/assert.h>

static void set_mouse_button_state(MouseButtonData* button, bool pressed_this_frame, Clay_Vector2 pointer_pos)
{
    if (pressed_this_frame) {
        if (button->state == MOUSE_BUTTON_PRESSED_THIS_FRAME) {
            button->state = MOUSE_BUTTON_PRESSED;
        } else if (button->state != MOUSE_BUTTON_PRESSED) {
            button->state = MOUSE_BUTTON_PRESSED_THIS_FRAME;
            button->click_origin.x = pointer_pos.x;
            button->click_origin.y = pointer_pos.y;
        }
    } else {
        if (button->state == MOUSE_BUTTON_RELEASED_THIS_FRAME) {
            button->state = MOUSE_BUTTON_RELEASED;
        } else if (button->state != MOUSE_BUTTON_RELEASED) {
            button->state = MOUSE_BUTTON_RELEASED_THIS_FRAME;
            button->click_origin.x = 0;
            button->click_origin.y = 0;
        }
    }
}

void mouse_state_reset(MouseState* mouse_state)
{
    mouse_state->scroll_delta = (Clay_Vector2) { 0 };
}

void mouse_state_update(MouseState* mouse_state, uint32_t buttons)
{
    set_mouse_button_state(
        &mouse_state->buttons[MOUSE_BUTTON_LEFT],
        buttons & MOUSE_BUTTON_LEFT_MASK,
        mouse_state->pos);

    set_mouse_button_state(
        &mouse_state->buttons[MOUSE_BUTTON_MIDDLE],
        buttons & MOUSE_BUTTON_MIDDLE_MASK,
        mouse_state->pos);

    set_mouse_button_state(
        &mouse_state->buttons[MOUSE_BUTTON_RIGHT],
        buttons & MOUSE_BUTTON_RIGHT_MASK,
        mouse_state->pos);

    set_mouse_button_state(
        &mouse_state->buttons[MOUSE_BUTTON_SIDE_1],
        buttons & MOUSE_BUTTON_SIDE_1_MASK,
        mouse_state->pos);

    set_mouse_button_state(
        &mouse_state->buttons[MOUSE_BUTTON_SIDE_2],
        buttons & MOUSE_BUTTON_SIDE_2_MASK,
        mouse_state->pos);
}

#define DRAG_SCROLL_MARGIN 48
#define DRAG_SCROLL_SPEED 1.f
#define MAX_DRAG_SCROLL_DELTA 1.f

static float clamped_scroll_delta_from_edge_distance(float distance_to_edge)
{
    float delta = (DRAG_SCROLL_MARGIN - distance_to_edge) * DRAG_SCROLL_SPEED / distance_to_edge;
    return CLAMP(delta, 0, MAX_DRAG_SCROLL_DELTA);
}

void Clay_Ext_UpdateScrollContainerCustom(
    Clay_ElementId container_id,
    Clay_ElementId scrollbar_h_id,
    Clay_ElementId scrollbar_v_id,
    Clay_Vector2 mouse_pos,
    Clay_Vector2 delta,
    ScrollContainerConfig config,
    const MouseState* mouse_state,
    DragState* drag_state)
{
    Clay_ScrollContainerData scroll = Clay_GetScrollContainerData(container_id);
    if (!scroll.found) {
        return;
    }

    const MouseButtonData mb_left = mouse_state->buttons[MOUSE_BUTTON_LEFT];

    const bool dragging = is_any_element_dragged(drag_state) && mb_left.state == MOUSE_BUTTON_PRESSED;
    const bool dragging_scrollbar_h = dragging && drag_state->active_id == scrollbar_h_id.id;
    const bool dragging_scrollbar_v = dragging && drag_state->active_id == scrollbar_v_id.id;
    const bool dragging_other = dragging && !dragging_scrollbar_h && !dragging_scrollbar_v;

    // Register currently dragged scrollbar if any
    if (!dragging) {
        if (Clay_PointerOver(scrollbar_h_id)) {
            drag_state->active_id = scrollbar_h_id.id;
            drag_state->click_origin = *scroll.scrollPosition;
        }
        if (Clay_PointerOver(scrollbar_v_id)) {
            drag_state->active_id = scrollbar_v_id.id;
            drag_state->click_origin = *scroll.scrollPosition;
        }
    }

    // Update container scroll position from scrollbar movement
    const Clay_Vector2 ratio = {
        .x = scroll.contentDimensions.width / scroll.scrollContainerDimensions.width,
        .y = scroll.contentDimensions.height / scroll.scrollContainerDimensions.height,
    };
    if (scroll.config.horizontal && dragging_scrollbar_h) {
        scroll.scrollPosition->x =
            drag_state->click_origin.x + ((mb_left.click_origin.x - mouse_state->pos.x) * ratio.x);
    }
    if (scroll.config.vertical && dragging_scrollbar_v) {
        scroll.scrollPosition->y =
            drag_state->click_origin.y + ((mb_left.click_origin.y - mouse_state->pos.y) * ratio.y);
    }

    const Clay_ElementData elem = Clay_GetElementData(container_id);
    if (!elem.found) {
        return;
    }
    const Clay_BoundingBox bb = elem.boundingBox;
    if (!point_inside_rect(mouse_pos, bb)) {
        return;
    }

    // Update container scroll position from mouse wheel movement
    if (config.use_both_wheels) {
        assert((scroll.config.horizontal != scroll.config.vertical) &&
            "Exactly one of horizontal/vertical scrolling should be enabled when use_both_wheels is true");
        if (scroll.config.horizontal) {
            delta.x += delta.y;
        } else if (scroll.config.vertical) {
            delta.y += delta.x;
        }
    }

    // Update container scroll position from dragging close to the container edges
    if (config.enable_drag_on_edges && dragging_other && point_inside_rect(mouse_pos, bb)) {
        // Scroll horizontally when close to left and right edges
        const float distance_to_left_edge = mouse_pos.x - bb.x;
        const float distance_to_right_edge = (bb.x + bb.width) - mouse_pos.x;
        if (distance_to_left_edge < DRAG_SCROLL_MARGIN) {
            delta.x += clamped_scroll_delta_from_edge_distance(distance_to_left_edge);
        } else if (distance_to_right_edge < DRAG_SCROLL_MARGIN) {
            delta.x -= clamped_scroll_delta_from_edge_distance(distance_to_right_edge);
        }
        // Scroll vertically when close to top and bottom edges
        const float distance_to_top_edge = mouse_pos.y - bb.y;
        const float distance_to_bottom_edge = (bb.y + bb.height) - mouse_pos.y;
        if (distance_to_top_edge < DRAG_SCROLL_MARGIN) {
            delta.y += clamped_scroll_delta_from_edge_distance(distance_to_top_edge);
        } else if (distance_to_bottom_edge < DRAG_SCROLL_MARGIN) {
            delta.y -= clamped_scroll_delta_from_edge_distance(distance_to_bottom_edge);
        }
    }

    // Apply scroll delta for mouse wheel and dragging close to the edges
    if (scroll.config.horizontal) {
        scroll.scrollPosition->x += delta.x * config.sensitivity;
        const float min_x = -MAX(scroll.contentDimensions.width - scroll.scrollContainerDimensions.width, 0);
        scroll.scrollPosition->x = CLAMP(scroll.scrollPosition->x, min_x, 0);
    }
    if (scroll.config.vertical) {
        scroll.scrollPosition->y += delta.y * config.sensitivity;
        const float min_y = -MAX(scroll.contentDimensions.height - scroll.scrollContainerDimensions.height, 0);
        scroll.scrollPosition->y = CLAMP(scroll.scrollPosition->y, min_y, 0);
    }
}
