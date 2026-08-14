#include "titlebar.h"

#if SDL_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#include <stdio.h>

#define PROP_MEDIT_WIN32_SDL_WNDPROC "medit.win32.sdl_wndproc"
#define PROP_MEDIT_WIN32_TITLEBAR_STATE "medit.win32.titlebar_state"

static LRESULT CALLBACK custom_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC sdl_wndproc = (WNDPROC)GetProp(hwnd, PROP_MEDIT_WIN32_SDL_WNDPROC);
    TitlebarState* titlebar_state = (TitlebarState*)GetProp(hwnd, PROP_MEDIT_WIN32_TITLEBAR_STATE);

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
                if (maximized) {
                    // avoid off-screen clip
                    p->rgrc[0].top += frameY;
                }
                return 0;
            }
        } break;
        case WM_NCHITTEST: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            SDL_Point point = { .x = pt.x, .y = pt.y };
            if (SDL_PointInRect(&point, &titlebar_state->maximize_button_rect)) {
                return HTMAXBUTTON;
            }
            if (SDL_PointInRect(&point, &titlebar_state->minimize_button_rect)) {
                return HTMINBUTTON;
            }
            if (SDL_PointInRect(&point, &titlebar_state->close_button_rect)) {
                return HTCLOSE;
            }
            if (pt.y >= 0 && pt.y < titlebar_state->height) {
                return HTCAPTION;
            }
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
            if (wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE) {
                return 0;
            }
        } break;
    }

    return CallWindowProc(sdl_wndproc, hwnd, msg, wParam, lParam);
}

bool SDL_SetWindowTitlebar(SDL_Window* window, TitlebarState* titlebar_state)
{
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!hwnd) {
        (void)fprintf(stderr, "No HWND\n");
        return false;
    }

    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    WNDPROC sdl_wndproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)custom_wndproc);
    SetProp(hwnd, PROP_MEDIT_WIN32_SDL_WNDPROC, (HANDLE)sdl_wndproc);
    SetProp(hwnd, PROP_MEDIT_WIN32_TITLEBAR_STATE, (HANDLE)titlebar_state);

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    return true;
}

#else

bool SDL_SetWindowTitlebar(SDL_Window* window, TitlebarState* titlebar_state)
{
    (void)window;
    (void)titlebar_state;
    return true;
}

#endif
