#include <core/meditor.h>

#include <core/assert.h>

#include <SDL3/SDL.h>

#if SDL_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#include <stdio.h>

#define PROP_MEDIT_WIN32_SDL_WNDPROC "medit.win32.sdl_wndproc"
#define PROP_MEDIT_WIN32_MEDITOR "medit.win32.meditor"

static Point win32_screen_to_client(HWND hwnd, LONG x, LONG y)
{
    POINT point = { x, y };
    assert(ScreenToClient(hwnd, &point) != 0);
    return (Point){ .x = point.x, .y = point.y };
}

static LRESULT CALLBACK custom_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC sdl_wndproc = (WNDPROC)GetProp(hwnd, PROP_MEDIT_WIN32_SDL_WNDPROC);
    Meditor* medit = (Meditor*)GetProp(hwnd, PROP_MEDIT_WIN32_MEDITOR);
    Theme* theme = &medit->config.theme;

    switch (msg) {
        case WM_NCCALCSIZE: {
            if (wParam == TRUE) {
                NCCALCSIZE_PARAMS *p = (NCCALCSIZE_PARAMS *)lParam;
                int frameX = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                int frameY = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                bool maximized = IsZoomed(hwnd);
                p->rgrc[0].left   += frameX;
                p->rgrc[0].right  -= frameX;
                p->rgrc[0].bottom -= frameY;
                if (maximized) p->rgrc[0].top += frameY; // avoid off-screen clip
                return 0;
            }
        } break;
        case WM_NCHITTEST: {
            Point point = win32_screen_to_client(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
            if (hit != HTCLIENT) return hit;
            // The default hit testing done by DefWindowProc never returns HTTOP* when the mouse is in the top border
            // due to the custom WM_NCCALCSIZE calculation
            if (point.y < theme->layout_settings.window_resize_border) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                if (point.x < theme->layout_settings.window_resize_border) return HTTOPLEFT;
                if (rect.right - point.x < theme->layout_settings.window_resize_border) return HTTOPRIGHT;
                return HTTOP;
            }
            if (medit_is_point_in_rect_pointers(&point, &medit->ui.titlebar_state.maximize_button_rect)) return HTMAXBUTTON;
            if (medit_is_point_in_rect_pointers(&point, &medit->ui.titlebar_state.minimize_button_rect)) return HTMINBUTTON;
            if (medit_is_point_in_rect_pointers(&point, &medit->ui.titlebar_state.close_button_rect)) return HTCLOSE;
            if (point.y >= 0 && point.y < theme->layout_settings.titlebar_height) return HTCAPTION;
            return HTCLIENT;
        } break;
        case WM_NCLBUTTONDOWN: {
            // Swallow left-button-down on our custom caption buttons.
            // Left to DefWindowProc, a caption hit-test (HTMIN/HTMAX/HTCLOSE) starts a modal button-tracking loop:
            // it captures the mouse and spins its own message loop until release, which freezes our main loop (and its
            // per-frame SDL_GetGlobalMouseState poll) and then fires WM_SYSCOMMAND itself, double-handling the click and
            // desyncing our edge detector. Returning 0 suppresses all of that so our portable poll is the single click authority.
            // NOTE: only swallow the button hit-tests, never HTCAPTION, that modal loop is the native drag-to-move we want
            // to keep. Snap Layouts are unaffected (they key off HTMAXBUTTON from WM_NCHITTEST on hover).
            if (wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE) return 0;
        } break;

        default: break;
    }

    return CallWindowProc(sdl_wndproc, hwnd, msg, wParam, lParam);
}

static bool titlebar_win32_init(SDL_Window* window, Meditor* medit)
{
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd) {
        (void)fprintf(stderr, "No HWND\n");
        return false;
    }

    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DEFAULT;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    WNDPROC sdl_wndproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)custom_wndproc);
    SetProp(hwnd, PROP_MEDIT_WIN32_SDL_WNDPROC, (HANDLE)sdl_wndproc);
    SetProp(hwnd, PROP_MEDIT_WIN32_MEDITOR, (HANDLE)medit);

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    return true;
}

#else

static SDL_HitTestResult handle_sdl_window_hit_test(SDL_Window *window, const SDL_Point *_area, void *_data)
{
    Meditor* medit = (Meditor*)_data;
    Theme* theme = &medit->config.theme;
    Point area = { .x = _area->x, .y = _area->y };

    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);

    const bool at_left      = area.x < theme->layout_settings.window_resize_border;
    const bool at_right     = area.x > width - theme->layout_settings.window_resize_border;
    const bool at_top       = area.y < theme->layout_settings.titlebar_height / 4;
    const bool at_title_bar = area.y < theme->layout_settings.titlebar_height;
    const bool at_bottom    = area.y > height - theme->layout_settings.window_resize_border;

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
        if (!medit_is_point_in_rect(area, medit->ui.titlebar_state.minimize_button_rect)
            && !medit_is_point_in_rect(area, medit->ui.titlebar_state.maximize_button_rect)
            && !medit_is_point_in_rect(area, medit->ui.titlebar_state.close_button_rect))
        {
            hit_test = SDL_HITTEST_DRAGGABLE;
        }
    }

    // printf("Window hit test: %d\n", hit_test);

    return hit_test;
}

#endif

static void titlebar_recalc_button_rects(Meditor* medit, int window_width)
{
    Theme* theme = &medit->config.theme;
    TitlebarState* state = &medit->ui.titlebar_state;

    state->minimize_button_rect = (Rect){
        .x = window_width - (theme->layout_settings.titlebar_button_width * 3),
        .y = 0,
        .w = theme->layout_settings.titlebar_button_width,
        .h = theme->layout_settings.titlebar_height,
    };

    state->maximize_button_rect = (Rect){
        .x = window_width - (theme->layout_settings.titlebar_button_width * 2),
        .y = 0,
        .w = theme->layout_settings.titlebar_button_width,
        .h = theme->layout_settings.titlebar_height,
    };

    state->close_button_rect = (Rect){
        .x = window_width - (theme->layout_settings.titlebar_button_width * 1),
        .y = 0,
        .w = theme->layout_settings.titlebar_button_width,
        .h = theme->layout_settings.titlebar_height,
    };
}

static TitlebarHoveredButton titlebar_button_at(Point point, const TitlebarState* state)
{
    if (medit_is_point_in_rect(point, state->minimize_button_rect)) {
        return TITLEBAR_BTN_MIN;
    }
    if (medit_is_point_in_rect(point, state->maximize_button_rect)) {
        return TITLEBAR_BTN_MAX;
    }
    if (medit_is_point_in_rect(point, state->close_button_rect)) {
        return TITLEBAR_BTN_CLOSE;
    }
    return TITLEBAR_BTN_NONE;
}

void medit_ui_titlebar_init(Meditor* medit, SDL_Window* window)
{
#if SDL_PLATFORM_WINDOWS
    // Note: SDL_SetWindowsMessageHook cannot be used here as it does not allow to inject events for SDL
    assert(titlebar_win32_init(window, medit));
#else
    assert_sdl(SDL_SetWindowHitTest(window, handle_sdl_window_hit_test, medit));
#endif
}

void medit_ui_update_titlebar(Meditor* medit, int window_width)
{
    titlebar_recalc_button_rects(medit, window_width);

    medit->ui.titlebar_state.hovered_button = titlebar_button_at(
        (Point){ medit->ui.mouse_state.pos.x, medit->ui.mouse_state.pos.y },
        &medit->ui.titlebar_state);
}

void medit_ui_layout_titlebar(Meditor* medit)
{
    Theme* theme = &medit->config.theme;

    CLAY(CLAY_ID("titlebar"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_FIXED((float)theme->layout_settings.titlebar_height),
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
                        .width = CLAY_SIZING_FIXED((float)theme->layout_settings.titlebar_button_width),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                .backgroundColor = to_clay_color(theme->color_scheme.titlebar_ctrl_button_minimize),
            });
            CLAY(CLAY_ID("titlebar_ctrl_button_max"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED((float)theme->layout_settings.titlebar_button_width),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                .backgroundColor = to_clay_color(theme->color_scheme.titlebar_ctrl_button_maximize),
            });
            CLAY(CLAY_ID("titlebar_ctrl_button_close"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED((float)theme->layout_settings.titlebar_button_width),
                        .height = CLAY_SIZING_GROW(0),
                    },
                },
                .backgroundColor = to_clay_color(theme->color_scheme.titlebar_ctrl_button_close),
            });
        }
    }
}
