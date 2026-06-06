#ifndef CLAY_UTILS_H_
#define CLAY_UTILS_H_

#include "clay.h"

//------------------------------------------------------------------------------
// Maths helpers
//------------------------------------------------------------------------------

static inline bool point_inside_rect(Clay_Vector2 point, Clay_BoundingBox rect)
{
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y
        && point.y <= rect.y + rect.height;
}

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

typedef enum {
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT,
    MOUSE_BUTTON_SIDE_1,
    MOUSE_BUTTON_SIDE_2,
    MOUSE_BUTTON_COUNT,
} MouseButtons;

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

#endif // CLAY_UTILS_H_
