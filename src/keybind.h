#ifndef MEDIT_KEYBIND_H_
#define MEDIT_KEYBIND_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Logical key representation - independent of physical layout
// Only usual keys from US QWERTY and French AZERTY layouts are supported
typedef enum {
    KEY_UNKNOWN,

    // clang-format off

    // Letters (A-Z)
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
    KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

    // Numbers (0-9)
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    // Function keys
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11,
    KEY_F12,

    // Special keys
    KEY_SPACE, KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB,
    KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_LEFT_CTRL, KEY_RIGHT_CTRL, KEY_LEFT_ALT, KEY_RIGHT_ALT,
    KEY_CTXMENU, KEY_CAPSLOCK, KEY_NUMLOCK,

    // Navigation
    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN,
    KEY_INSERT, KEY_DELETE,

    // Punctuation
    KEY_MINUS, KEY_EQUALS,
    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
    KEY_LEFT_PAREN, KEY_RIGHT_PAREN,
    KEY_LEFT_BRACE, KEY_RIGHT_BRACE,
    KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_BACKSLASH, KEY_GRAVE, KEY_COLON,
    KEY_EXCLAM, KEY_ASTERISK, KEY_DOLLAR, KEY_UGRAVE, KEY_SUPERSCRIPT2,

    // Numpad
    KEY_NPAD_0, KEY_NPAD_1, KEY_NPAD_2, KEY_NPAD_3, KEY_NPAD_4,
    KEY_NPAD_5, KEY_NPAD_6, KEY_NPAD_7, KEY_NPAD_8, KEY_NPAD_9,
    KEY_NPAD_DIVIDE, KEY_NPAD_MULTIPLY, KEY_NPAD_MINUS, KEY_NPAD_PLUS, KEY_NPAD_ENTER,
    KEY_NPAD_PERIOD,

    // clang-format on

    KEY_COUNT,
} Key;

typedef enum {
    MOD_NONE = 0,
    MOD_SHIFT = 1 << 0,
    MOD_CTRL = 1 << 1,
    MOD_ALT = 1 << 2,
    MOD_SHIFT_CTRL = MOD_SHIFT | MOD_CTRL,
    MOD_SHIFT_ALT = MOD_SHIFT | MOD_ALT,
    MOD_CTRL_ALT = MOD_CTRL | MOD_ALT,
    MOD_SHIFT_CTRL_ALT = MOD_SHIFT | MOD_CTRL | MOD_ALT,
    MOD_MASK = MOD_SHIFT_CTRL_ALT,
    MOD_COUNT = MOD_MASK + 1,
} Keymod;

typedef enum {
    EVENT_UNKNOWN = 0,
    EVENT_KEY_PRESS,
    EVENT_KEY_RELEASE,
    EVENT_KEY_REPEAT
} KeybindEventType;

typedef struct {
    KeybindEventType type;
    Key key;
    uint32_t scancode; // Physical key position (platform-specific)
    uint32_t modifiers; // Bitfield of Modifiers
    void* platform_data; // Platform-specific data
} KeybindEvent;

#define MAX_KEY_STATES 256

#if KEY_COUNT > MAX_KEY_STATES
#error "KEY_COUNT must not exceed MAX_KEY_STATES"
#endif

typedef struct Meditor Meditor;

typedef void(KeyActionFn)(Meditor* medit, void* userdata);

typedef struct {
    KeyActionFn* callback;
    Meditor* medit;
    void* userdata;
} KeybindEntry;

typedef struct {
    KeybindEntry bindings[MAX_KEY_STATES][MOD_COUNT];
} Keybind;

void keybind_reinit(Keybind* keybind);

bool keybind_bind(
    Keybind* keybind,
    Key key,
    uint32_t modifiers,
    KeyActionFn* callback,
    Meditor* medit,
    void* userdata);

void keybind_unbind(Keybind* keybind, Key key, uint32_t modifiers);

const KeybindEntry* keybind_get(const Keybind* keybind, Key key, uint32_t modifiers);

bool keybind_handle_event(Keybind* keybind, const KeybindEvent* event);

const char* keybind_key_to_string(Key key);

#endif // MEDIT_KEYBIND_H_
