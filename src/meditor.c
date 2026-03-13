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
}

void meditor_load_default_tui_keybind(Meditor* medit)
{
    (void)medit;
}

void meditor_cursor_up(Meditor* medit, int cells)
{
    medit->cursor_row -= cells;
    if (medit->cursor_row < 0) {
        medit->cursor_row = 0;
    }
}

void meditor_cursor_down(Meditor* medit, int cells)
{
    medit->cursor_row += cells;
    if (medit->cursor_row >= medit->grid_rows) {
        medit->cursor_row = medit->grid_rows - 1;
    }
}

void meditor_cursor_left(Meditor* medit, int cells)
{
    medit->cursor_col -= cells;
    if (medit->cursor_col < 0) {
        medit->cursor_col = 0;
    }
}

void meditor_cursor_right(Meditor* medit, int cells)
{
    medit->cursor_col += cells;
    if (medit->cursor_col >= medit->grid_cols) {
        medit->cursor_col = medit->grid_cols - 1;
    }
    if (medit->cursor_col >= medit->text_cells) {
        medit->cursor_col = medit->text_cells;
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
