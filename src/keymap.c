#include "keymap.h"

#include "assert.h"

#include <string.h>

static KeyMap keymaps[KEYMOD_COUNT];

void reset_keymaps()
{
    memset(keymaps, 0, sizeof(keymaps));
}

KeyMap* get_keymap(KeyMod keymod)
{
    assert(keymod < KEYMOD_COUNT);
    return &keymaps[keymod];
}

void set_keymap_action(Key key, KeyMapHandler* handler)
{
    KeyMap* keymap = get_keymap(key.mod);
    assert(keymap);
    assert(key.code < KEYCODE_COUNT);
    (*keymap)[key.code] = handler;
}

KeyMapHandler* get_keymap_action(Key key)
{
    KeyMap* keymap = get_keymap(key.mod);
    assert(keymap);
    assert(key.code < KEYCODE_COUNT);
    return (*keymap)[key.code];
}