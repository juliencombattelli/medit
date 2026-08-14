#pragma once

#include "clay_utils.h"

#include <core/assert.h>

#include <SDL3/SDL.h>

enum {
    TITLEBAR_BTN_NONE,
    TITLEBAR_BTN_MIN,
    TITLEBAR_BTN_MAX,
    TITLEBAR_BTN_CLOSE,
};

typedef struct {
    int height;
    int resize_border;
    int button_width;
    int hovered_button;
    SDL_Rect minimize_button_rect;
    SDL_Rect maximize_button_rect;
    SDL_Rect close_button_rect;
} TitlebarState;

void titlebar_init(TitlebarState* titlebar_state, SDL_Window* window);

void titlebar_update(TitlebarState* titlebar_state, SDL_Window* window, MouseState* mouse_state, bool* running);

void titlebar_layout(TitlebarState* state);
