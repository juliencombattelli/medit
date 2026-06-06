#include "clay_utils.h"

#include <assert.h>

static void set_mouse_button_state(
    MouseButtonData* button,
    bool pressed_this_frame,
    Clay_Vector2 pointer_pos)
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

bool Clay_Ext_UpdateScrollContainerCustom(
    Clay_ElementId id,
    Clay_Vector2 mouse_pos,
    Clay_Vector2 delta,
    ScrollContainerData scroll_container_data,
    bool dragging)
{
    if (delta.x == 0 && delta.y == 0 && !dragging) {
        return false;
    }

    Clay_ElementData elem = Clay_GetElementData(id);
    if (!elem.found) {
        return false;
    }
    Clay_BoundingBox bb = elem.boundingBox;
    if (!point_inside_rect(mouse_pos, bb)) {
        return false;
    }

    Clay_ScrollContainerData scroll = Clay_GetScrollContainerData(id);
    if (!scroll.found) {
        return false;
    }

    if (scroll_container_data.use_both_wheels) {
        assert(
            (scroll.config.horizontal != scroll.config.vertical)
            && "Exactly one of horizontal/vertical scrolling should be enabled when "
               "use_both_wheels is true");
        if (scroll.config.horizontal) {
            delta.x += delta.y;
        } else if (scroll.config.vertical) {
            delta.y += delta.x;
        }
    }

    if (dragging) {
#define DRAG_SCROLL_MARGIN 48
#define DRAG_SCROLL_SPEED 0.1f
        // If dragging, apply additional scroll when close to the edges to allow dragging beyond the
        // current view. The closer to the edge, the faster the scroll.
        float distance_to_left_edge = mouse_pos.x - bb.x;
        float distance_to_right_edge = (bb.x + bb.width) - mouse_pos.x;
        if (distance_to_left_edge < DRAG_SCROLL_MARGIN) {
            delta.x += (DRAG_SCROLL_MARGIN - distance_to_left_edge) * DRAG_SCROLL_SPEED
                / distance_to_left_edge;
        } else if (distance_to_right_edge < DRAG_SCROLL_MARGIN) {
            delta.x -= (DRAG_SCROLL_MARGIN - distance_to_right_edge) * DRAG_SCROLL_SPEED
                / distance_to_right_edge;
        }
    }

    const float scale = scroll_container_data.sensitivity * 10.0f;

    if (scroll.config.horizontal) {
        scroll.scrollPosition->x += delta.x * scale;
        float min_x = -MAX(
            scroll.contentDimensions.width - scroll.scrollContainerDimensions.width,
            0);
        scroll.scrollPosition->x = CLAMP(scroll.scrollPosition->x, min_x, 0);
    }
    if (scroll.config.vertical) {
        scroll.scrollPosition->y += delta.y * scale;
        float min_y = -MAX(
            scroll.contentDimensions.height - scroll.scrollContainerDimensions.height,
            0);
        scroll.scrollPosition->y = CLAMP(scroll.scrollPosition->y, min_y, 0);
    }
    return true;
}

void Clay_Ext_UpdateScrollContainerFromScrollbar(
    ScrollbarDragState* sbs,
    MouseState* mouse,
    Clay_ElementId scrollbar_id,
    Clay_ElementId container_id)
{
    if (mouse->buttons[MOUSE_BUTTON_LEFT].state != MOUSE_BUTTON_PRESSED
        && mouse->buttons[MOUSE_BUTTON_LEFT].state != MOUSE_BUTTON_PRESSED_THIS_FRAME) {
        if (sbs->active_id == scrollbar_id.id) {
            sbs->active_id = 0;
        }
        return;
    }

    Clay_ScrollContainerData scroll_data = Clay_GetScrollContainerData(container_id);
    if (!scroll_data.found) {
        return;
    }

    if (sbs->active_id == NO_DRAGGED_SCOLLBAR
        && mouse->buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_PRESSED_THIS_FRAME
        && Clay_PointerOver(scrollbar_id)) {
        sbs->active_id = scrollbar_id.id;
        sbs->click_origin = *scroll_data.scrollPosition;
    } else if (sbs->active_id == scrollbar_id.id) {
        Clay_Vector2 ratio = {
            scroll_data.contentDimensions.width / scroll_data.scrollContainerDimensions.width,
            scroll_data.contentDimensions.height / scroll_data.scrollContainerDimensions.height,
        };
        if (scroll_data.config.horizontal) {
            scroll_data.scrollPosition->x = sbs->click_origin.x
                + (mouse->buttons[MOUSE_BUTTON_LEFT].click_origin.x - mouse->pos.x) * ratio.x;
        }
        if (scroll_data.config.vertical) {
            scroll_data.scrollPosition->y = sbs->click_origin.y
                + (mouse->buttons[MOUSE_BUTTON_LEFT].click_origin.y - mouse->pos.y) * ratio.y;
        }
    }
}
