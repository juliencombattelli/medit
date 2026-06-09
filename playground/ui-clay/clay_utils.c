#include "clay_utils.h"

#include <assert.h>

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

// Core scroll update logic shared by both wheel and scrollbar scrolling
static void apply_scroll_delta(Clay_ScrollContainerData* scroll, Clay_Vector2 delta, float sensitivity)
{
    if (scroll->config.horizontal) {
        scroll->scrollPosition->x += delta.x * sensitivity;
        float min_x = -MAX(scroll->contentDimensions.width - scroll->scrollContainerDimensions.width, 0);
        scroll->scrollPosition->x = CLAMP(scroll->scrollPosition->x, min_x, 0);
    }
    if (scroll->config.vertical) {
        scroll->scrollPosition->y += delta.y * sensitivity;
        float min_y = -MAX(scroll->contentDimensions.height - scroll->scrollContainerDimensions.height, 0);
        scroll->scrollPosition->y = CLAMP(scroll->scrollPosition->y, min_y, 0);
    }
}

#define DRAG_SCROLL_MARGIN 48
#define DRAG_SCROLL_SPEED 1.f
#define MAX_DRAG_SCROLL_DELTA 1.f

void Clay_Ext_UpdateScrollContainerCustom(
    Clay_ElementId container_id,
    Clay_ElementId scrollbar_id,
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

    bool dragging = is_any_element_dragged(drag_state);

    MouseButtonData mb_left = mouse_state->buttons[MOUSE_BUTTON_LEFT];

    // Update container scroll position from scrollbar movement
    if (mb_left.state != MOUSE_BUTTON_PRESSED && mb_left.state != MOUSE_BUTTON_PRESSED_THIS_FRAME) {
        if (drag_state->active_id == scrollbar_id.id) {
            drag_state->active_id = CLAY_EXT_NULL_ID;
        }
    } else {
        if (!dragging && mb_left.state == MOUSE_BUTTON_PRESSED_THIS_FRAME && Clay_PointerOver(scrollbar_id)) {
            drag_state->active_id = scrollbar_id.id;
            drag_state->click_origin = *scroll.scrollPosition;
            return;
        }
        if (drag_state->active_id == scrollbar_id.id) {
            Clay_Vector2 ratio = {
                .x = scroll.contentDimensions.width / scroll.scrollContainerDimensions.width,
                .y = scroll.contentDimensions.height / scroll.scrollContainerDimensions.height,
            };
            // TODO use apply_scroll_delta to handle the config switches in one place
            if (scroll.config.horizontal) {
                scroll.scrollPosition->x =
                    drag_state->click_origin.x + (mb_left.click_origin.x - mouse_state->pos.x) * ratio.x;
            }
            if (scroll.config.vertical) {
                scroll.scrollPosition->y =
                    drag_state->click_origin.y + (mb_left.click_origin.y - mouse_state->pos.y) * ratio.y;
            }
            return;
        }
    }

    // Update container scroll position from mouse wheel movement and dragging close to the edges
    if (delta.x == 0 && delta.y == 0 && !dragging) {
        return;
    }

    Clay_ElementData elem = Clay_GetElementData(container_id);
    if (!elem.found) {
        return;
    }

    Clay_BoundingBox bb = elem.boundingBox;
    if (!point_inside_rect(mouse_pos, bb)) {
        return;
    }

    if (config.use_both_wheels) {
        assert((scroll.config.horizontal != scroll.config.vertical) &&
            "Exactly one of horizontal/vertical scrolling should be enabled when use_both_wheels is true");
        if (scroll.config.horizontal) {
            delta.x += delta.y;
        } else if (scroll.config.vertical) {
            delta.y += delta.x;
        }
    }

    // TODO support it on X and Y
    if (config.enable_drag_on_edges && dragging) {
        float distance_to_left_edge = mouse_pos.x - bb.x;
        float distance_to_right_edge = (bb.x + bb.width) - mouse_pos.x;
        if (distance_to_left_edge < DRAG_SCROLL_MARGIN) {
            float scroll_delta =
                (DRAG_SCROLL_MARGIN - distance_to_left_edge) * DRAG_SCROLL_SPEED / distance_to_left_edge;
            delta.x += CLAMP(scroll_delta, 0, MAX_DRAG_SCROLL_DELTA);
        } else if (distance_to_right_edge < DRAG_SCROLL_MARGIN) {
            float scroll_delta =
                (DRAG_SCROLL_MARGIN - distance_to_right_edge) * DRAG_SCROLL_SPEED / distance_to_right_edge;
            delta.x -= CLAMP(scroll_delta, 0, MAX_DRAG_SCROLL_DELTA);
        }
    }

    apply_scroll_delta(&scroll, delta, config.sensitivity);
}
