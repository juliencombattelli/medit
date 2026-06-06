#ifndef CLAY_UTILS_H_
#define CLAY_UTILS_H_

#include "clay.h"

#include <math.h>

//------------------------------------------------------------------------------
// Maths helpers
//------------------------------------------------------------------------------

static inline bool point_inside_rect(Clay_Vector2 point, Clay_BoundingBox rect)
{
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y
        && point.y <= rect.y + rect.height;
}

static inline float distance(Clay_Vector2 a, Clay_Vector2 b)
{
    return sqrtf(powf((b.x - a.x), 2) + powf((b.y - a.y), 2));
}

#define CLAMP(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

//------------------------------------------------------------------------------
// Mouse state handling (pointer, buttons & scroll wheels)
//------------------------------------------------------------------------------

typedef enum {
    // A mouse button is not currently down / was released at some point in the past.
    MOUSE_BUTTON_RELEASED,
    // A mouse button click was released this frame.
    MOUSE_BUTTON_RELEASED_THIS_FRAME,
    // A mouse button click occurred in a previous frame and is still currently held down this
    // frame.
    MOUSE_BUTTON_PRESSED,
    // A mouse button click occurred this frame.
    MOUSE_BUTTON_PRESSED_THIS_FRAME,
} MouseButtonState;

#define MOUSE_BUTTON_LEFT 0u
#define MOUSE_BUTTON_MIDDLE 1u
#define MOUSE_BUTTON_RIGHT 2u
#define MOUSE_BUTTON_SIDE_1 3u
#define MOUSE_BUTTON_SIDE_2 4u
#define MOUSE_BUTTON_COUNT 5u

#define MOUSE_BUTTON_MASK(button) (1u << (unsigned)(button))

#define MOUSE_BUTTON_LEFT_MASK MOUSE_BUTTON_MASK(MOUSE_BUTTON_LEFT)
#define MOUSE_BUTTON_MIDDLE_MASK MOUSE_BUTTON_MASK(MOUSE_BUTTON_MIDDLE)
#define MOUSE_BUTTON_RIGHT_MASK MOUSE_BUTTON_MASK(MOUSE_BUTTON_RIGHT)
#define MOUSE_BUTTON_SIDE_1_MASK MOUSE_BUTTON_MASK(MOUSE_BUTTON_SIDE_1)
#define MOUSE_BUTTON_SIDE_2_MASK MOUSE_BUTTON_MASK(MOUSE_BUTTON_SIDE_2)

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

// Element scrolling handling

typedef struct {
    float sensitivity;
    bool use_both_wheels;
} ScrollContainerData;

// Update a single scroll container with a custom algorithm.
// Apply wheel scroll to a specific element with a custom sensitivity and potential axis
// combination. Clay internally multiplies delta by 10, so we replicate that here, sensitivity = 1.0
// matches Clay's default speed. Returns true if the mouse was over the element and the scroll was
// applied. Also applies scroll when a dragging element is close to the edges.
bool Clay_Ext_UpdateScrollContainerCustom(
    Clay_ElementId id,
    Clay_Vector2 mouse_pos,
    Clay_Vector2 delta,
    ScrollContainerData scroll_container_data,
    bool dragging);

//------------------------------------------------------------------------------
// Scrollbar state handling
//------------------------------------------------------------------------------

#define NO_DRAGGED_SCOLLBAR (0u)

// State shared by all scrollbars in the UI because only one scrollbar can be active at a time
// TODO this should be extended to any draggable element, no just scrollbars
typedef struct {
    Clay_Vector2 click_origin;
    uint32_t active_id;
} ScrollbarDragState;

// Call once per scrollbar per frame. scrollbar_id is the draggable thumb element;
// container_id is the associated clip/scroll container.
void Clay_Ext_UpdateScrollContainerFromScrollbar(
    ScrollbarDragState* sbs,
    MouseState* mouse,
    Clay_ElementId scrollbar_id,
    Clay_ElementId container_id);

#endif // CLAY_UTILS_H_
