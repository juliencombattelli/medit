#ifndef MEDIT_UI_SDL3_SDL3_H_
#define MEDIT_UI_SDL3_SDL3_H_

#include <core/keybind.h>
#include <core/meditor.h>

KeybindEvent keybind_sdl3_translate_event(void* native_event);

void medit_ui_sdl3_run(Meditor* medit);

#endif // MEDIT_UI_SDL3_SDL3_H_
