#ifndef CLAY_UTILS_H_
#define CLAY_UTILS_H_

#include "clay.h"

#include <math.h>
#include <string.h>

#define CLAY_EXT_NULL_ID (0u) // ID 0 is internally reserved for null IDs in Clay
#define CLAY_EXT_NULL_ELEMENT_ID (Clay_ElementId) { 0 }

static inline Clay_String Clay_Ext_StringFromCStr(const char* str) {
    return (Clay_String) {
        .isStaticallyAllocated = true,
        .length = str ? (int32_t)strlen(str) : 0,
        .chars = str,
    };
}


//------------------------------------------------------------------------------
// Maths helpers
//------------------------------------------------------------------------------

static inline bool point_inside_rect(Clay_Vector2 point, Clay_BoundingBox rect)
{
    return point.x >= rect.x && point.x <= rect.x + rect.width
        && point.y >= rect.y && point.y <= rect.y + rect.height;
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

static inline bool is_any_element_dragged(DragState* drag_state)
{
    return drag_state->active_id != CLAY_EXT_NULL_ID;
}

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

#endif // CLAY_UTILS_H_
