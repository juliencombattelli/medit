#include "meditor.h"

#include <string.h>

void set_font_size_clamped(int* font, int value)
{
    if (value > FONT_SIZE_MAX) {
        value = FONT_SIZE_MAX;
    }
    if (value < FONT_SIZE_MIN) {
        value = FONT_SIZE_MIN;
    }
    *font = value;
}

void action_toggle_debug_grid(Meditor* medit)
{
    medit->draw_debug_grid = !medit->draw_debug_grid;
}

void action_font_zoom_out(Meditor* medit)
{
    set_font_size_clamped(&medit->editor_font_size, medit->editor_font_size - 2);
    medit_unload_font(medit);
    medit_load_font(medit);
}

void action_font_zoom_in(Meditor* medit)
{
    set_font_size_clamped(&medit->editor_font_size, medit->editor_font_size + 2);
    medit_unload_font(medit);
    medit_load_font(medit);
}

void action_cursor_up(Meditor* medit)
{
    meditor_cursor_up(medit, 1);
}

void action_cursor_down(Meditor* medit)
{
    meditor_cursor_down(medit, 1);
}

void action_cursor_left(Meditor* medit)
{
    meditor_cursor_left(medit, 1);
}

void action_cursor_right(Meditor* medit)
{
    meditor_cursor_right(medit, 1);
}

void action_restore_cursor(Meditor* medit)
{
    medit->cursor_index = 0;
    // TODO memset other cursors
}

void action_add_cursor_down(Meditor* medit)
{
    Vec2* prev_cursor = &medit->cursor_pos[medit->cursor_index];
    Vec2* new_cursor = &medit->cursor_pos[++medit->cursor_index];
    *new_cursor = vec2(prev_cursor->x, prev_cursor->y + 1);
}

void meditor_load_default_gui_keybind(Meditor* medit)
{
    Keybind* keybind = &medit->keybind;

    keybind_bind(keybind, KEY_A, MOD_CTRL, action_toggle_debug_grid, medit);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, action_font_zoom_in, medit);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, action_font_zoom_in, medit);

    keybind_bind(keybind, KEY_NPAD_MINUS, MOD_CTRL, action_font_zoom_out, medit);
    keybind_bind(keybind, KEY_6, MOD_CTRL, action_font_zoom_out, medit);

    keybind_bind(keybind, KEY_UP, MOD_NONE, action_cursor_up, medit);
    keybind_bind(keybind, KEY_DOWN, MOD_NONE, action_cursor_down, medit);
    keybind_bind(keybind, KEY_LEFT, MOD_NONE, action_cursor_left, medit);
    keybind_bind(keybind, KEY_RIGHT, MOD_NONE, action_cursor_right, medit);

    keybind_bind(keybind, KEY_ESCAPE, MOD_NONE, action_restore_cursor, medit);
    keybind_bind(keybind, KEY_DOWN, MOD_CTRL_ALT, action_add_cursor_down, medit);
}

void meditor_load_default_tui_keybind(Meditor* medit)
{
    (void)medit;
}

void meditor_cursor_up(Meditor* medit, int cells)
{
    for (size_t c = 0; c <= medit->cursor_index; ++c) {
        Vec2* cursor = &medit->cursor_pos[c];
        cursor->row -= cells;
        if (cursor->row < 0) {
            cursor->row = 0;
        }
    }
}

void meditor_cursor_down(Meditor* medit, int cells)
{
    for (size_t c = 0; c <= medit->cursor_index; ++c) {
        Vec2* cursor = &medit->cursor_pos[c];
        cursor->row += cells;
        if (cursor->row >= medit->grid_size.row) {
            cursor->row = medit->grid_size.row - 1;
        }
    }
}

void meditor_cursor_left(Meditor* medit, int cells)
{
    for (size_t c = 0; c <= medit->cursor_index; ++c) {
        Vec2* cursor = &medit->cursor_pos[c];
        cursor->col -= cells;
        if (cursor->col < 0) {
            cursor->col = 0;
        }
    }
}

void meditor_cursor_right(Meditor* medit, int cells)
{
    for (size_t c = 0; c <= medit->cursor_index; ++c) {
        Vec2* cursor = &medit->cursor_pos[c];
        cursor->col += cells;
        if (cursor->col >= medit->grid_size.col) {
            cursor->col = medit->grid_size.col - 1;
        }
        if (cursor->col >= medit->text_cells) {
            cursor->col = medit->text_cells;
        }
    }
}

void meditor_append_text(Meditor* medit, const char* text, int cells)
{
    const size_t input_size = strlen(text);
    const size_t free_space = TEXT_CAPACITY - medit->text_size;
    if (medit->text_size > free_space) {
        medit->text_size = free_space;
    }
    memcpy(medit->text + medit->text_size, text, input_size);
    medit->text_size += input_size;
    medit->text_cells += cells;
}
