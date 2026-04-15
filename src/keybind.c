#include "keybind.h"
#include "assert.h"

#include <stdio.h>

void keybind_reinit(Keybind* keybind)
{
    for (int key = 0; key < MAX_KEY_STATES; key++) {
        for (int mod = 0; mod < MOD_COUNT; mod++) {
            keybind->bindings[key][mod] = (KeybindEntry) { 0 };
        }
    }
}

bool keybind_bind(
    Keybind* keybind,
    Key key,
    uint32_t modifiers,
    KeyActionFn* callback,
    Meditor* medit,
    void* userdata)
{
    if (key >= KEY_COUNT) {
        return false;
    }

    uint32_t mod_index = modifiers & MOD_MASK;
    keybind->bindings[key][mod_index] = (KeybindEntry) {
        .callback = callback,
        .medit = medit,
        .userdata = userdata,
    };
    return true;
}

void keybind_unbind(Keybind* keybind, Key key, uint32_t modifiers)
{
    if (key >= KEY_COUNT) {
        return;
    }

    uint32_t mod_index = modifiers & MOD_MASK;
    keybind->bindings[key][mod_index] = (KeybindEntry) { 0 };
}

const KeybindEntry* keybind_get(const Keybind* keybind, Key key, uint32_t modifiers)
{
    if (key >= KEY_COUNT) {
        return NULL;
    }

    uint32_t mod_index = modifiers & MOD_MASK;
    const KeybindEntry* entry = &keybind->bindings[key][mod_index];

    return entry;
}

bool keybind_handle_event(Keybind* keybind, const KeybindEvent* event)
{
    printf("[DEBUG] Key: %s\n", keybind_key_to_string(event->key));

    const KeybindEntry* entry = keybind_get(keybind, event->key, event->modifiers);
    if (entry->callback != NULL) {
        entry->callback(entry->medit, entry->userdata);
        return true;
    }

    return false;
}

const char* keybind_key_to_string(Key key)
{
    switch (key) {
        case KEY_UNKNOWN: return "Unknown";

        case KEY_A: return "A";
        case KEY_B: return "B";
        case KEY_C: return "C";
        case KEY_D: return "D";
        case KEY_E: return "E";
        case KEY_F: return "F";
        case KEY_G: return "G";
        case KEY_H: return "H";
        case KEY_I: return "I";
        case KEY_J: return "J";
        case KEY_K: return "K";
        case KEY_L: return "L";
        case KEY_M: return "M";
        case KEY_N: return "N";
        case KEY_O: return "O";
        case KEY_P: return "P";
        case KEY_Q: return "Q";
        case KEY_R: return "R";
        case KEY_S: return "S";
        case KEY_T: return "T";
        case KEY_U: return "U";
        case KEY_V: return "V";
        case KEY_W: return "W";
        case KEY_X: return "X";
        case KEY_Y: return "Y";
        case KEY_Z: return "Z";

        case KEY_0: return "0";
        case KEY_1: return "1";
        case KEY_2: return "2";
        case KEY_3: return "3";
        case KEY_4: return "4";
        case KEY_5: return "5";
        case KEY_6: return "6";
        case KEY_7: return "7";
        case KEY_8: return "8";
        case KEY_9: return "9";

        case KEY_SPACE: return "Space";
        case KEY_ENTER: return "Enter";
        case KEY_ESCAPE: return "Escape";
        case KEY_BACKSPACE: return "Backspace";
        case KEY_TAB: return "Tab";

        case KEY_LEFT: return "Left";
        case KEY_RIGHT: return "Right";
        case KEY_UP: return "Up";
        case KEY_DOWN: return "Down";

        case KEY_F1: return "F1";
        case KEY_F2: return "F2";
        case KEY_F3: return "F3";
        case KEY_F4: return "F4";
        case KEY_F5: return "F5";
        case KEY_F6: return "F6";
        case KEY_F7: return "F7";
        case KEY_F8: return "F8";
        case KEY_F9: return "F9";
        case KEY_F10: return "F10";
        case KEY_F11: return "F11";
        case KEY_F12: return "F12";

        case KEY_LEFT_SHIFT: return "Left Shift";
        case KEY_RIGHT_SHIFT: return "Right Shift";
        case KEY_LEFT_CTRL: return "Left Ctrl";
        case KEY_RIGHT_CTRL: return "Right Ctrl";
        case KEY_LEFT_ALT: return "Left Alt";
        case KEY_RIGHT_ALT: return "Right Alt";
        case KEY_CTXMENU: return "Context Menu";
        case KEY_CAPSLOCK: return "Caps Lock";
        case KEY_NUMLOCK: return "Num Lock";

        case KEY_HOME: return "Home";
        case KEY_END: return "End";
        case KEY_PAGE_UP: return "Page Up";
        case KEY_PAGE_DOWN: return "Page Down";
        case KEY_INSERT: return "Insert";
        case KEY_DELETE: return "Delete";
        case KEY_MINUS: return "-";
        case KEY_EQUALS: return "=";
        case KEY_LEFT_BRACKET: return "[";
        case KEY_RIGHT_BRACKET: return "]";
        case KEY_SEMICOLON: return ";";
        case KEY_QUOTE: return "'";
        case KEY_COMMA: return ",";
        case KEY_PERIOD: return ".";
        case KEY_SLASH: return "/";
        case KEY_BACKSLASH: return "\\";
        case KEY_GRAVE: return "Grave";
        case KEY_LEFT_PAREN: return "(";
        case KEY_RIGHT_PAREN: return ")";
        case KEY_LEFT_BRACE: return "{";
        case KEY_RIGHT_BRACE: return "}";
        case KEY_COLON: return ":";
        case KEY_EXCLAM: return "!";
        case KEY_ASTERISK: return "*";
        case KEY_DOLLAR: return "$";
        case KEY_UGRAVE: return "ù";
        case KEY_SUPERSCRIPT2: return "²";

        case KEY_NPAD_0: return "Numpad 0";
        case KEY_NPAD_1: return "Numpad 1";
        case KEY_NPAD_2: return "Numpad 2";
        case KEY_NPAD_3: return "Numpad 3";
        case KEY_NPAD_4: return "Numpad 4";
        case KEY_NPAD_5: return "Numpad 5";
        case KEY_NPAD_6: return "Numpad 6";
        case KEY_NPAD_7: return "Numpad 7";
        case KEY_NPAD_8: return "Numpad 8";
        case KEY_NPAD_9: return "Numpad 9";
        case KEY_NPAD_DIVIDE: return "Numpad Divide";
        case KEY_NPAD_MULTIPLY: return "Numpad Multiply";
        case KEY_NPAD_MINUS: return "Numpad Minus";
        case KEY_NPAD_PLUS: return "Numpad Plus";
        case KEY_NPAD_ENTER: return "Numpad Enter";
        case KEY_NPAD_PERIOD: return "Numpad Period";

        case KEY_COUNT: break;
    }
    assert(false && "Key out of range");
}
