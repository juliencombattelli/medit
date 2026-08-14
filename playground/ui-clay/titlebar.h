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

bool SDL_SetWindowTitlebar(SDL_Window* window, TitlebarState* titlebar_state);

static inline SDL_HitTestResult handle_sdl_window_hit_test(SDL_Window *window, const SDL_Point *area, void *data)
{
    TitlebarState* titlebar_state = (TitlebarState*)data;

    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);

    const bool at_left      = area->x < titlebar_state->resize_border;
    const bool at_right     = area->x > width - titlebar_state->resize_border;
    const bool at_top       = area->y < titlebar_state->height / 4;
    const bool at_title_bar = area->y < titlebar_state->height;
    const bool at_bottom    = area->y > height - titlebar_state->resize_border;

    SDL_HitTestResult hit_test = SDL_HITTEST_NORMAL;

    if (at_top) {
        if (at_left) {
            hit_test = SDL_HITTEST_RESIZE_TOPLEFT;
        } else if (at_right) {
            hit_test = SDL_HITTEST_RESIZE_TOPRIGHT;
        } else {
            hit_test = SDL_HITTEST_RESIZE_TOP;
        }
    } else if (at_bottom) {
        if (at_left) {
            hit_test = SDL_HITTEST_RESIZE_BOTTOMLEFT;
        } else if (at_right) {
            hit_test = SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        } else {
            hit_test = SDL_HITTEST_RESIZE_BOTTOM;
        }
    } else if (at_left) {
        hit_test = SDL_HITTEST_RESIZE_LEFT;
    } else if (at_right) {
        hit_test = SDL_HITTEST_RESIZE_RIGHT;
    } else if (at_title_bar) {
        if (!SDL_PointInRect(area, &titlebar_state->minimize_button_rect)
            && !SDL_PointInRect(area, &titlebar_state->maximize_button_rect)
            && !SDL_PointInRect(area, &titlebar_state->close_button_rect))
        {
            hit_test = SDL_HITTEST_DRAGGABLE;
        }
    }

    // printf("Window hit test: %d\n", hit_test);

    return hit_test;
}

static inline void titlebar_recalc_button_rects(TitlebarState* state, int window_width)
{
    state->minimize_button_rect = (SDL_Rect){
        .x = window_width - (state->button_width * 3),
        .y = 0,
        .w = state->button_width,
        .h = state->height
    };

    state->maximize_button_rect = (SDL_Rect){
        .x = window_width - (state->button_width * 2),
        .y = 0,
        .w = state->button_width,
        .h = state->height
    };

    state->close_button_rect = (SDL_Rect){
        .x = window_width - (state->button_width * 1),
        .y = 0,
        .w = state->button_width,
        .h = state->height
    };
}

static inline int titlebar_button_at(SDL_Point point, const TitlebarState* state)
{
    if (SDL_PointInRect(&point, &state->minimize_button_rect)) {
        return TITLEBAR_BTN_MIN;
    }
    if (SDL_PointInRect(&point, &state->maximize_button_rect)) {
        return TITLEBAR_BTN_MAX;
    }
    if (SDL_PointInRect(&point, &state->close_button_rect)) {
        return TITLEBAR_BTN_CLOSE;
    }
    return TITLEBAR_BTN_NONE;
}


static inline void titlebar_update_hovered_button(TitlebarState* state, SDL_Window* window)
{
    float gx = 0, gy = 0;
    SDL_GetGlobalMouseState(&gx, &gy);

    int wx = 0, wy = 0;
    SDL_GetWindowPosition(window, &wx, &wy);

    int mx = (int)gx - wx;
    int my = (int)gy - wy;

    state->hovered_button = titlebar_button_at((SDL_Point){mx, my}, state);
}

static inline void titlebar_init(TitlebarState* titlebar_state, SDL_Window* window)
{
#if SDL_PLATFORM_WINDOWS
    assert(SDL_SetWindowTitlebar(window, titlebar_state));
#else
    assert_sdl(SDL_SetWindowHitTest(window, handle_sdl_window_hit_test, titlebar_state));
#endif
}

static inline void titlebar_update(TitlebarState* titlebar_state, SDL_Window* window, MouseState* mouse_state, bool* running)
{
    int window_width = 0;
    SDL_GetWindowSize(window, &window_width, NULL);
    titlebar_recalc_button_rects(titlebar_state, window_width);

    titlebar_update_hovered_button(titlebar_state, window);

    if (mouse_state->buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_RELEASED_THIS_FRAME) {
        if (titlebar_state->hovered_button == TITLEBAR_BTN_MIN) {
            SDL_MinimizeWindow(window);
        }
        if (titlebar_state->hovered_button == TITLEBAR_BTN_MAX) {
            if ((SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0) {
                SDL_RestoreWindow(window);
            } else {
                SDL_MaximizeWindow(window);
            }
        }
        if (titlebar_state->hovered_button == TITLEBAR_BTN_CLOSE) {
            *running = false;
        }
    }
}

static inline void titlebar_layout(TitlebarState* state)
{
    CLAY(CLAY_ID("titlebar"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIXED((float)state->height),
            },
        },
        .backgroundColor = { 0x7F, 0x00, 0x7F, 0xFF},
    }) {
        CLAY(CLAY_ID("titlebar_ctrl_buttons"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0),
                },
            },
        }) {
            CLAY(CLAY_ID("titlebar_filler"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW(0),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
            });
            CLAY(CLAY_ID("titlebar_ctrl_button_min"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED((float)state->button_width),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                .backgroundColor = { 0xFF, 0xFF, 0x00, 0xFF},
            });
            CLAY(CLAY_ID("titlebar_ctrl_button_max"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED((float)state->button_width),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                .backgroundColor = { 0xFF, 0x00, 0xFF, 0xFF},
            });
            CLAY(CLAY_ID("titlebar_ctrl_button_close"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED((float)state->button_width),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                .backgroundColor = { 0x00, 0xFF, 0xFF, 0xFF},
            });
        }
    }
}
