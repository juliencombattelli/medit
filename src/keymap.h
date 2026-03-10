#ifndef MEDTI_KEYMAP_H_
#define MEDTI_KEYMAP_H_

// The Medit keyboard key codes representation.
// Based on SDL scancodes (SDL_scancode.h).
// The values in this enumeration are based on the USB usage page standard:
// https://usb.org/sites/default/files/hut1_5.pdf

typedef enum {
    KEYCODE_UNKNOWN = 0x00,

    KEYCODE_A = 0x04,
    KEYCODE_B = 0x05,
    KEYCODE_C = 0x06,
    KEYCODE_D = 0x07,
    KEYCODE_E = 0x08,
    KEYCODE_F = 0x09,
    KEYCODE_G = 0x0A,
    KEYCODE_H = 0x0B,
    KEYCODE_I = 0x0C,
    KEYCODE_J = 0x0D,
    KEYCODE_K = 0x0E,
    KEYCODE_L = 0x0F,
    KEYCODE_M = 0x10,
    KEYCODE_N = 0x11,
    KEYCODE_O = 0x12,
    KEYCODE_P = 0x13,
    KEYCODE_Q = 0x14,
    KEYCODE_R = 0x15,
    KEYCODE_S = 0x16,
    KEYCODE_T = 0x17,
    KEYCODE_U = 0x18,
    KEYCODE_V = 0x19,
    KEYCODE_W = 0x1A,
    KEYCODE_X = 0x1B,
    KEYCODE_Y = 0x1C,
    KEYCODE_Z = 0x1D,

    KEYCODE_1 = 0x1E,
    KEYCODE_2 = 0x1F,
    KEYCODE_3 = 0x20,
    KEYCODE_4 = 0x21,
    KEYCODE_5 = 0x22,
    KEYCODE_6 = 0x23,
    KEYCODE_7 = 0x24,
    KEYCODE_8 = 0x25,
    KEYCODE_9 = 0x26,
    KEYCODE_0 = 0x27,

    KEYCODE_RETURN = 0x28,
    KEYCODE_ESCAPE = 0x29,
    KEYCODE_BACKSPACE = 0x2A,
    KEYCODE_TAB = 0x2B,
    KEYCODE_SPACE = 0x2C,

    KEYCODE_MINUS = 0x2D,
    KEYCODE_EQUALS = 0x2E,
    KEYCODE_LEFTBRACKET = 0x2F,
    KEYCODE_RIGHTBRACKET = 0x30,
    KEYCODE_BACKSLASH = 0x31,
    // Located at the lower left of the return key on ISO keyboards and at the
    // right end of the QWERTY row on ANSI keyboards. Produces REVERSE
    // SOLIDUS(backslash) and VERTICAL LINE in a US layout, REVERSE SOLIDUS and
    // VERTICAL LINE in a UK Mac layout, NUMBER SIGN and TILDE in a UK Windows
    // layout, DOLLAR SIGN and POUND SIGN in a Swiss German layout, NUMBER SIGN
    // and APOSTROPHE in a German layout, GRAVE ACCENT and POUND SIGN in a
    // French Mac layout, and ASTERISK and MICRO SIGN in a French Windows
    // layout.

    KEYCODE_NONUSHASH = 0x32,
    // ISO USB keyboards actually use this code instead of 49 for the same key,
    // but all OSes I've seen treat the two codes identically. So, as an
    // implementor, unless your keyboard generates both of those codes and your
    // OS treats them differently, you should generate KEYCODE_BACKSLASH instead
    // of this code. As a user, you should not rely on this code because SDL
    // will never generate it with most (all?) keyboards.

    KEYCODE_SEMICOLON = 0x33,
    KEYCODE_APOSTROPHE = 0x34,
    KEYCODE_GRAVE = 0x35,
    // Located in the top left corner (on both ANSI and ISO keyboards). Produces
    // GRAVE ACCENT and TILDE in a US Windows layout and in US and UK Mac
    // layouts on ANSI keyboards, GRAVE ACCENT and NOT SIGN in a UK Windows
    // layout, SECTION SIGN and PLUS-MINUS SIGN in US and UK Mac layouts on ISO
    // keyboards, SECTION SIGN and DEGREE SIGN in a Swiss German layout (Mac:
    // only on ISO keyboards), CIRCUMFLEX ACCENT and DEGREE SIGN in a German
    // layout (Mac: only on ISO keyboards), SUPERSCRIPT TWO and TILDE in a
    // French Windows layout, COMMERCIAL AT and NUMBER SIGN in a French Mac
    // layout on ISO keyboards, and LESS-THAN SIGN and GREATER-THAN SIGN in a
    // Swiss German, German, or French Mac layout on ANSI keyboards.

    KEYCODE_COMMA = 0x36,
    KEYCODE_PERIOD = 0x37,
    KEYCODE_SLASH = 0x38,

    KEYCODE_CAPSLOCK = 0x39,

    KEYCODE_F1 = 0x3A,
    KEYCODE_F2 = 0x3B,
    KEYCODE_F3 = 0x3C,
    KEYCODE_F4 = 0x3D,
    KEYCODE_F5 = 0x3E,
    KEYCODE_F6 = 0x3F,
    KEYCODE_F7 = 0x40,
    KEYCODE_F8 = 0x41,
    KEYCODE_F9 = 0x42,
    KEYCODE_F10 = 0x43,
    KEYCODE_F11 = 0x44,
    KEYCODE_F12 = 0x45,

    KEYCODE_PRINTSCREEN = 0x46,
    KEYCODE_SCROLLLOCK = 0x47,
    KEYCODE_PAUSE = 0x48,
    KEYCODE_INSERT = 0x49, // insert on PC, help on some Mac keyboards (but does
                           // send code 73, not 117)
    KEYCODE_HOME = 0x4A,
    KEYCODE_PAGEUP = 0x4B,
    KEYCODE_DELETE = 0x4C,
    KEYCODE_END = 0x4D,
    KEYCODE_PAGEDOWN = 0x4E,
    KEYCODE_RIGHT = 0x4F,
    KEYCODE_LEFT = 0x50,
    KEYCODE_DOWN = 0x51,
    KEYCODE_UP = 0x52,

    KEYCODE_NUMLOCKCLEAR = 0x53, // num lock on PC, clear on Mac keyboards
    KEYCODE_KP_DIVIDE = 0x54,
    KEYCODE_KP_MULTIPLY = 0x55,
    KEYCODE_KP_MINUS = 0x56,
    KEYCODE_KP_PLUS = 0x57,
    KEYCODE_KP_ENTER = 0x58,
    KEYCODE_KP_1 = 0x59,
    KEYCODE_KP_2 = 0x5A,
    KEYCODE_KP_3 = 0x5B,
    KEYCODE_KP_4 = 0x5C,
    KEYCODE_KP_5 = 0x5D,
    KEYCODE_KP_6 = 0x5E,
    KEYCODE_KP_7 = 0x5F,
    KEYCODE_KP_8 = 0x60,
    KEYCODE_KP_9 = 0x61,
    KEYCODE_KP_0 = 0x62,
    KEYCODE_KP_PERIOD = 0x63,

    KEYCODE_NONUSBACKSLASH = 0x64,
    // This is the additional key that ISO keyboards have over ANSI ones,
    // located between left shift and Z. Produces GRAVE ACCENT and TILDE in a US
    // or UK Mac layout, REVERSE SOLIDUS (backslash) and VERTICAL LINE in a US
    // or UK Windows layout, and LESS-THAN SIGN and GREATER-THAN SIGN in a Swiss
    // German, German, or French layout.

    KEYCODE_APPLICATION = 0x65, // windows contextual menu, compose
    KEYCODE_POWER = 0x66,
    // The USB document says this is a status flag, not a physical key - but
    // some Mac keyboards do have a power key.
    KEYCODE_KP_EQUALS = 0x67,
    KEYCODE_F13 = 0x68,
    KEYCODE_F14 = 0x69,
    KEYCODE_F15 = 0x6A,
    KEYCODE_F16 = 0x6B,
    KEYCODE_F17 = 0x6C,
    KEYCODE_F18 = 0x6D,
    KEYCODE_F19 = 0x6E,
    KEYCODE_F20 = 0x6F,
    KEYCODE_F21 = 0x70,
    KEYCODE_F22 = 0x71,
    KEYCODE_F23 = 0x72,
    KEYCODE_F24 = 0x73,
    KEYCODE_EXECUTE = 0x74,
    KEYCODE_HELP = 0x75, // AL Integrated Help Center
    KEYCODE_MENU = 0x76, // Menu (show menu)
    KEYCODE_SELECT = 0x77,
    KEYCODE_STOP = 0x78, // AC Stop
    KEYCODE_AGAIN = 0x79, // AC Redo/Repeat
    KEYCODE_UNDO = 0x7A, // AC Undo
    KEYCODE_CUT = 0x7B, // AC Cut
    KEYCODE_COPY = 0x7C, // AC Copy
    KEYCODE_PASTE = 0x7D, // AC Paste
    KEYCODE_FIND = 0x7E, // AC Find
    KEYCODE_MUTE = 0x7F,
    KEYCODE_VOLUMEUP = 0x80,
    KEYCODE_VOLUMEDOWN = 0x81,

    KEYCODE_KP_COMMA = 0x85,
    KEYCODE_KP_EQUALSAS400 = 0x86,

    KEYCODE_INTERNATIONAL1 = 0x87, // used on Asian keyboards, see footnotes in
                                   // USB doc
    KEYCODE_INTERNATIONAL2 = 0x88,
    KEYCODE_INTERNATIONAL3 = 0x89, // Yen
    KEYCODE_INTERNATIONAL4 = 0x8A,
    KEYCODE_INTERNATIONAL5 = 0x8B,
    KEYCODE_INTERNATIONAL6 = 0x8C,
    KEYCODE_INTERNATIONAL7 = 0x8D,
    KEYCODE_INTERNATIONAL8 = 0x8E,
    KEYCODE_INTERNATIONAL9 = 0x8F,
    KEYCODE_LANG1 = 0x90, // Hangul/English toggle
    KEYCODE_LANG2 = 0x91, // Hanja conversion
    KEYCODE_LANG3 = 0x92, // Katakana
    KEYCODE_LANG4 = 0x93, // Hiragana
    KEYCODE_LANG5 = 0x94, // Zenkaku/Hankaku
    KEYCODE_LANG6 = 0x95, // reserved
    KEYCODE_LANG7 = 0x96, // reserved
    KEYCODE_LANG8 = 0x97, // reserved
    KEYCODE_LANG9 = 0x98, // reserved

    KEYCODE_ALTERASE = 0x99, // Erase-Eaze
    KEYCODE_SYSREQ = 0x9A,
    KEYCODE_CANCEL = 0x9B, // AC Cancel
    KEYCODE_CLEAR = 0x9C,
    KEYCODE_PRIOR = 0x9D,
    KEYCODE_RETURN2 = 0x9E,
    KEYCODE_SEPARATOR = 0x9F,
    KEYCODE_OUT = 0xA0,
    KEYCODE_OPER = 0xA1,
    KEYCODE_CLEARAGAIN = 0xA2,
    KEYCODE_CRSEL = 0xA3,
    KEYCODE_EXSEL = 0xA4,

    KEYCODE_KP_00 = 0xB0,
    KEYCODE_KP_000 = 0xB1,
    KEYCODE_THOUSANDSSEPARATOR = 0xB2,
    KEYCODE_DECIMALSEPARATOR = 0xB3,
    KEYCODE_CURRENCYUNIT = 0xB4,
    KEYCODE_CURRENCYSUBUNIT = 0xB5,
    KEYCODE_KP_LEFTPAREN = 0xB6,
    KEYCODE_KP_RIGHTPAREN = 0xB7,
    KEYCODE_KP_LEFTBRACE = 0xB8,
    KEYCODE_KP_RIGHTBRACE = 0xB9,
    KEYCODE_KP_TAB = 0xBA,
    KEYCODE_KP_BACKSPACE = 0xBB,
    KEYCODE_KP_A = 0xBC,
    KEYCODE_KP_B = 0xBD,
    KEYCODE_KP_C = 0xBE,
    KEYCODE_KP_D = 0xBF,
    KEYCODE_KP_E = 0xC0,
    KEYCODE_KP_F = 0xC1,
    KEYCODE_KP_XOR = 0xC2,
    KEYCODE_KP_POWER = 0xC3,
    KEYCODE_KP_PERCENT = 0xC4,
    KEYCODE_KP_LESS = 0xC5,
    KEYCODE_KP_GREATER = 0xC6,
    KEYCODE_KP_AMPERSAND = 0xC7,
    KEYCODE_KP_DBLAMPERSAND = 0xC8,
    KEYCODE_KP_VERTICALBAR = 0xC9,
    KEYCODE_KP_DBLVERTICALBAR = 0xCA,
    KEYCODE_KP_COLON = 0xCB,
    KEYCODE_KP_HASH = 0xCC,
    KEYCODE_KP_SPACE = 0xCD,
    KEYCODE_KP_AT = 0xCE,
    KEYCODE_KP_EXCLAM = 0xCF,
    KEYCODE_KP_MEMSTORE = 0xD0,
    KEYCODE_KP_MEMRECALL = 0xD1,
    KEYCODE_KP_MEMCLEAR = 0xD2,
    KEYCODE_KP_MEMADD = 0xD3,
    KEYCODE_KP_MEMSUBTRACT = 0xD4,
    KEYCODE_KP_MEMMULTIPLY = 0xD5,
    KEYCODE_KP_MEMDIVIDE = 0xD6,
    KEYCODE_KP_PLUSMINUS = 0xD7,
    KEYCODE_KP_CLEAR = 0xD8,
    KEYCODE_KP_CLEARENTRY = 0xD9,
    KEYCODE_KP_BINARY = 0xDA,
    KEYCODE_KP_OCTAL = 0xDB,
    KEYCODE_KP_DECIMAL = 0xDC,
    KEYCODE_KP_HEXADECIMAL = 0xDD,

    KEYCODE_LCTRL = 0xE0,
    KEYCODE_LSHIFT = 0xE1,
    KEYCODE_LALT = 0xE2, // alt, option
    KEYCODE_LGUI = 0xE3, // windows, command (apple), meta
    KEYCODE_RCTRL = 0xE4,
    KEYCODE_RSHIFT = 0xE5,
    KEYCODE_RALT = 0xE6, // alt gr, option
    KEYCODE_RGUI = 0xE7, // windows, command (apple), meta

    KEYCODE_COUNT = 0xFF // not a key, marks the total count of key codes
} KeyCode;

typedef enum {
    KEYMOD_NONE = 0u,
    KEYMOD_CTRL = 1u << 0u,
    KEYMOD_SHIFT = 1u << 1u,
    KEYMOD_ALT = 1u << 2u,
    KEYMOD_COUNT = (KEYMOD_CTRL | KEYMOD_SHIFT | KEYMOD_ALT) + 1,
} KeyMod;

typedef struct {
    KeyCode code;
    KeyMod mod;
} Key;

struct RendererSDL;
struct Meditor;

typedef void(
    KeyMapHandler)(struct RendererSDL* renderer, struct Meditor* medit);

typedef KeyMapHandler*(KeyMap)[KEYCODE_COUNT];

// typedef size_t (IntoKeyCode)()

typedef struct KeyMaps {
    KeyMap mods[KEYMOD_COUNT];

} KeyMaps;

void reset_keymaps();

KeyMap* get_keymap(KeyMod keymod);

void set_keymap_action(Key key, KeyMapHandler* handler);

KeyMapHandler* get_keymap_action(Key key);

#endif // MEDTI_KEYMAP_H_
