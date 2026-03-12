#include "keybind_sdl3.h"

#include <SDL3/SDL.h>

static Key sdl3_keycode_to_keybind_key(SDL_Keycode keycode)
{
    switch (keycode) {
        case SDLK_A:
            return KEY_A;
        case SDLK_B:
            return KEY_B;
        case SDLK_C:
            return KEY_C;
        case SDLK_D:
            return KEY_D;
        case SDLK_E:
            return KEY_E;
        case SDLK_F:
            return KEY_F;
        case SDLK_G:
            return KEY_G;
        case SDLK_H:
            return KEY_H;
        case SDLK_I:
            return KEY_I;
        case SDLK_J:
            return KEY_J;
        case SDLK_K:
            return KEY_K;
        case SDLK_L:
            return KEY_L;
        case SDLK_M:
            return KEY_M;
        case SDLK_N:
            return KEY_N;
        case SDLK_O:
            return KEY_O;
        case SDLK_P:
            return KEY_P;
        case SDLK_Q:
            return KEY_Q;
        case SDLK_R:
            return KEY_R;
        case SDLK_S:
            return KEY_S;
        case SDLK_T:
            return KEY_T;
        case SDLK_U:
            return KEY_U;
        case SDLK_V:
            return KEY_V;
        case SDLK_W:
            return KEY_W;
        case SDLK_X:
            return KEY_X;
        case SDLK_Y:
            return KEY_Y;
        case SDLK_Z:
            return KEY_Z;

        case SDLK_0:
            return KEY_0;
        case SDLK_1:
            return KEY_1;
        case SDLK_2:
            return KEY_2;
        case SDLK_3:
            return KEY_3;
        case SDLK_4:
            return KEY_4;
        case SDLK_5:
            return KEY_5;
        case SDLK_6:
            return KEY_6;
        case SDLK_7:
            return KEY_7;
        case SDLK_8:
            return KEY_8;
        case SDLK_9:
            return KEY_9;

        case SDLK_SPACE:
            return KEY_SPACE;
        case SDLK_RETURN:
            return KEY_ENTER;
        case SDLK_ESCAPE:
            return KEY_ESCAPE;
        case SDLK_BACKSPACE:
            return KEY_BACKSPACE;
        case SDLK_TAB:
            return KEY_TAB;

        case SDLK_LSHIFT:
            return KEY_LEFT_SHIFT;
        case SDLK_RSHIFT:
            return KEY_RIGHT_SHIFT;
        case SDLK_LCTRL:
            return KEY_LEFT_CTRL;
        case SDLK_RCTRL:
            return KEY_RIGHT_CTRL;
        case SDLK_LALT:
            return KEY_LEFT_ALT;
        case SDLK_RALT:
            return KEY_RIGHT_ALT;
        case SDLK_APPLICATION:
            return KEY_CTXMENU;
        case SDLK_CAPSLOCK:
            return KEY_CAPSLOCK;
        case SDLK_NUMLOCKCLEAR:
            return KEY_NUMLOCK;

        case SDLK_LEFT:
            return KEY_LEFT;
        case SDLK_RIGHT:
            return KEY_RIGHT;
        case SDLK_UP:
            return KEY_UP;
        case SDLK_DOWN:
            return KEY_DOWN;

        case SDLK_HOME:
            return KEY_HOME;
        case SDLK_END:
            return KEY_END;
        case SDLK_PAGEUP:
            return KEY_PAGE_UP;
        case SDLK_PAGEDOWN:
            return KEY_PAGE_DOWN;
        case SDLK_INSERT:
            return KEY_INSERT;
        case SDLK_DELETE:
            return KEY_DELETE;

        case SDLK_F1:
            return KEY_F1;
        case SDLK_F2:
            return KEY_F2;
        case SDLK_F3:
            return KEY_F3;
        case SDLK_F4:
            return KEY_F4;
        case SDLK_F5:
            return KEY_F5;
        case SDLK_F6:
            return KEY_F6;
        case SDLK_F7:
            return KEY_F7;
        case SDLK_F8:
            return KEY_F8;
        case SDLK_F9:
            return KEY_F9;
        case SDLK_F10:
            return KEY_F10;
        case SDLK_F11:
            return KEY_F11;
        case SDLK_F12:
            return KEY_F12;

        case SDLK_MINUS:
            return KEY_MINUS;
        case SDLK_EQUALS:
            return KEY_EQUALS;
        case SDLK_LEFTBRACKET:
            return KEY_LEFT_BRACKET;
        case SDLK_RIGHTBRACKET:
            return KEY_RIGHT_BRACKET;
        case SDLK_LEFTPAREN:
            return KEY_LEFT_PAREN;
        case SDLK_RIGHTPAREN:
            return KEY_RIGHT_PAREN;
        case SDLK_LEFTBRACE:
            return KEY_LEFT_BRACE;
        case SDLK_RIGHTBRACE:
            return KEY_RIGHT_BRACE;
        case SDLK_SEMICOLON:
            return KEY_SEMICOLON;
        case SDLK_APOSTROPHE:
            return KEY_QUOTE;
        case SDLK_COMMA:
            return KEY_COMMA;
        case SDLK_PERIOD:
            return KEY_PERIOD;
        case SDLK_SLASH:
            return KEY_SLASH;
        case SDLK_BACKSLASH:
            return KEY_BACKSLASH;
        case SDLK_GRAVE:
            return KEY_GRAVE;
        case SDLK_COLON:
            return KEY_COLON;
        case SDLK_EXCLAIM:
            return KEY_EXCLAM;
        case SDLK_ASTERISK:
            return KEY_ASTERISK;
        case SDLK_DOLLAR:
            return KEY_DOLLAR;

        case SDLK_KP_0:
            return KEY_NPAD_0;
        case SDLK_KP_1:
            return KEY_NPAD_1;
        case SDLK_KP_2:
            return KEY_NPAD_2;
        case SDLK_KP_3:
            return KEY_NPAD_3;
        case SDLK_KP_4:
            return KEY_NPAD_4;
        case SDLK_KP_5:
            return KEY_NPAD_5;
        case SDLK_KP_6:
            return KEY_NPAD_6;
        case SDLK_KP_7:
            return KEY_NPAD_7;
        case SDLK_KP_8:
            return KEY_NPAD_8;
        case SDLK_KP_9:
            return KEY_NPAD_9;
        case SDLK_KP_PLUS:
            return KEY_NPAD_PLUS;
        case SDLK_KP_MINUS:
            return KEY_NPAD_MINUS;
        case SDLK_KP_DIVIDE:
            return KEY_NPAD_DIVIDE;
        case SDLK_KP_MULTIPLY:
            return KEY_NPAD_MULTIPLY;
        case SDLK_KP_ENTER:
            return KEY_NPAD_ENTER;
        case SDLK_KP_PERIOD:
            return KEY_NPAD_PERIOD;

        case 0xB2:
            return KEY_SUPERSCRIPT2;
        case 0xF9:
            return KEY_UGRAVE;

        default:
            return KEY_UNKNOWN;
    }
}

static uint32_t sdl3_keymod_to_keybind_mod(SDL_Keymod sdl_mods)
{
    uint32_t result = MOD_NONE;
    if (sdl_mods & SDL_KMOD_SHIFT) {
        result |= MOD_SHIFT;
    }
    if (sdl_mods & SDL_KMOD_CTRL) {
        result |= MOD_CTRL;
    }
    if (sdl_mods & SDL_KMOD_ALT) {
        result |= MOD_ALT;
    }
    return result;
}

KeybindEvent keybind_sdl3_translate_event(void* native_event)
{
    KeybindEvent result = { 0 };

    if (!native_event) {
        return result;
    }

    result.platform_data = native_event;

    SDL_Event* sdl_event = (SDL_Event*)native_event;

    if (sdl_event->type == SDL_EVENT_KEY_DOWN) {
        result.type = sdl_event->key.repeat ? EVENT_KEY_REPEAT
                                            : EVENT_KEY_PRESS;
    } else if (sdl_event->type == SDL_EVENT_KEY_UP) {
        result.type = EVENT_KEY_RELEASE;
    } else {
        return result;
    }

    result.key = sdl3_keycode_to_keybind_key(sdl_event->key.key);
    result.scancode = sdl_event->key.scancode;
    result.modifiers = sdl3_keymod_to_keybind_mod(sdl_event->key.mod);

    return result;
}
