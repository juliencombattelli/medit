#ifndef MEDIT_PLATFORM_SDL3_SDL3_H_
#define MEDIT_PLATFORM_SDL3_SDL3_H_

#include "keybind.h"
#include "renderer.h"

Renderer renderer_sdl3(void);

KeybindEvent keybind_sdl3_translate_event(void* native_event);

#endif // MEDIT_PLATFORM_SDL3_SDL3_H_
