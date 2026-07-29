#include "sdl3_ext_win32.h"

#if SDL_PLATFORM_WINDOWS

#include <windows.h>
#include <dwmapi.h>

static void sdl3_ext_win32_extend_frame(HWND hwnd, size_t title_bar_height)
{
    MARGINS margins = { 0, 0, title_bar_height, 0 };
    // cxLeftWidth, cxRightWidth, cyTopHeight, cyBottomHeight

    HRESULT hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
    if (FAILED(hr)) {
        SDL_Log("DwmExtendFrameIntoClientArea failed: 0x%08lx", (unsigned long)hr);
    }

    // Optional: enable dark-mode-aware border color on Win10/11
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */,
                           &dark, sizeof(dark));
}


static bool SDLCALL sdl3_ext_win32_message_hook(void *userdata, MSG *msg)
{
    size_t title_bar_height = (size_t)userdata;
    switch (msg->message) {
        case WM_ACTIVATE:
        case WM_DWMCOMPOSITIONCHANGED:
            sdl3_ext_win32_extend_frame(msg->hwnd, title_bar_height);
            break;
        default:
            break;
    }
    return true; // let SDL continue processing the message
}

void SDL_Ext_SetWindowsMessageHook(SDL_Window* window, size_t title_bar_height)
{

    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = (HWND)SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    if (hwnd) {
        sdl3_ext_win32_extend_frame(hwnd, title_bar_height);
        SDL_SetWindowsMessageHook(sdl3_ext_win32_message_hook, (void*)title_bar_height);
    } else {
        SDL_Log("Could not retrieve native HWND");
    }
}

#else

void SDL_Ext_SetWindowsMessageHook(SDL_Window* window, size_t title_bar_height)
{
    (void)window;
    (void)title_bar_height;
}

#endif
