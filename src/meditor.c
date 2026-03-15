#include "meditor.h"
#include "dynarray.h"

#include <string.h>

#define MEDIT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MEDIT_MAX(a, b) ((a) > (b) ? (a) : (b))

#define MEDIT_LINE_DEFAULT_CAPACITY 1024

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

void action_quit(Meditor* medit)
{
    medit->running = false;
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

void action_dump_state(Meditor* medit)
{
    printf("Dump state:\n");
    printf(
        "  cursor: c=%d, r=%d; lines:%d\n  lines:\n",
        medit->cursor_pos[0].col,
        medit->cursor_pos[0].row,
        medit->focused_view.file->lines.count);
    Lines* lines = &medit->focused_view.file->lines;
    int row = 0;
    dynarray_foreach(Line, line, lines)
    {
        if (line->count != 0) {
            printf("    #%d:`%.*s`\n", row++, line->count, line->items);
        } else {
            printf("    #%d:``\n", row++);
        }
    }
}

void action_erase_line(Meditor* medit)
{
    meditor_erase_line(medit);
}

void meditor_load_default_gui_keybind(Meditor* medit)
{
    Keybind* keybind = &medit->keybind;

    keybind_bind(keybind, KEY_Q, MOD_CTRL, action_quit, medit);

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

    keybind_bind(keybind, KEY_D, MOD_CTRL, action_dump_state, medit);

    keybind_bind(keybind, KEY_K, MOD_SHIFT_CTRL, action_erase_line, medit);
}

void meditor_load_default_tui_keybind(Meditor* medit)
{
    (void)medit;
}

static void fixup_cursor_col(Meditor* medit)
{
    // Adjust cursor column position when switching line
    Line* line = meditor_get_current_line(medit);
    size_t row_length = line->count;
    Vec2* cursor = &medit->cursor_pos[0];
    cursor->col = MEDIT_MIN(cursor->col, row_length);
}

void meditor_cursor_up(Meditor* medit, int cells)
{
    Vec2* cursor = &medit->cursor_pos[0];
    if (cursor->row > 0) {
        --cursor->row;
    }
    fixup_cursor_col(medit);
    // TODO handle multi cursor
    // for (size_t c = 0; c <= medit->cursor_index; ++c) {
    //     Vec2* cursor = &medit->cursor_pos[c];
    //     cursor->row -= cells;
    //     if (cursor->row < 0) {
    //         cursor->row = 0;
    //     }
    // }
}

void meditor_cursor_down(Meditor* medit, int cells)
{
    size_t line_count = medit->focused_view.file->lines.count;
    Vec2* cursor = &medit->cursor_pos[0];
    if (cursor->row < line_count - 1) {
        ++cursor->row;
    }
    fixup_cursor_col(medit);
    // TODO handle multi cursor
    // for (size_t c = 0; c <= medit->cursor_index; ++c) {
    //     Vec2* cursor = &medit->cursor_pos[c];
    //     cursor->row += cells;
    //     if (cursor->row >= medit->grid_size.row) {
    //         cursor->row = medit->grid_size.row - 1;
    //     }
    // }
}

void meditor_cursor_left(Meditor* medit, int cells)
{
    Line* line = meditor_get_current_line(medit);
    Vec2* cursor = &medit->cursor_pos[0];
    if (cursor->col > 0) {
        --cursor->col;
    } else if (cursor->row > 0) {
        // TODO handle going to upper row
    }
    // TODO handle multi cursor
    // for (size_t c = 0; c <= medit->cursor_index; ++c) {
    //     Vec2* cursor = &medit->cursor_pos[c];
    //     cursor->col -= cells;
    //     if (cursor->col < 0) {
    //         cursor->col = 0;
    //     }
    // }
}

void meditor_cursor_right(Meditor* medit, int cells)
{
    Line* line = meditor_get_current_line(medit);
    Vec2* cursor = &medit->cursor_pos[0];
    if (cursor->col < line->count) {
        ++cursor->col;
    } else if (cursor->row < 0) {
        // TODO handle going to lower row
    }
    // TODO handle multi cursor
    // for (size_t c = 0; c <= medit->cursor_index; ++c) {
    //     Vec2* cursor = &medit->cursor_pos[c];
    //     cursor->col += cells;
    //     if (cursor->col >= medit->grid_size.col) {
    //         cursor->col = medit->grid_size.col - 1;
    //     }
    //     if (cursor->col >= medit->text_cells) {
    //         cursor->col = medit->text_cells;
    //     }
    // }
}

void meditor_split_line(Meditor* medit)
{
    const int cursor_col = medit->cursor_pos[0].col;
    Line* current_line = meditor_get_current_line(medit);
    meditor_insert_new_line(medit);
    meditor_cursor_down(medit, 0);
    if (current_line->count != 0) {
        meditor_insert_text(
            medit,
            &current_line->items[cursor_col],
            current_line->count - cursor_col,
            0);
        memset(&current_line->items[cursor_col], 0, current_line->count - cursor_col);
        current_line->count = cursor_col;
    }
}

void meditor_insert_text(Meditor* medit, const char* text, int n, int cells)
{
    const int cursor_col = medit->cursor_pos[0].col;

    Line* current_line = meditor_get_current_line(medit);

    dynarray_insert_many(current_line, text, n, cursor_col);
}

void meditor_new_file(Meditor* medit)
{
    File new_file = { 0 };
    dynarray_append(&medit->opened_files, new_file);

    FileView new_file_view = {
        .file = &dynarray_last(&medit->opened_files),
    };
    dynarray_append(&medit->file_views, new_file_view);
    medit->focused_view = dynarray_last(&medit->file_views);

    meditor_insert_new_line(medit);
}

void meditor_insert_new_line(Meditor* medit)
{
    // TODO verify if next line is empty with a non-null capacity for reuse

    Lines* lines = &medit->focused_view.file->lines;

    Line empty_line = { 0 };
    dynarray_reserve(&empty_line, MEDIT_LINE_DEFAULT_CAPACITY);

    int line_pos = lines->count > 0 ? medit->cursor_pos[0].row + 1 : 0;

    dynarray_insert(lines, empty_line, line_pos);
}

Line* meditor_get_current_line(Meditor* medit)
{
    int cursor_row = medit->cursor_pos[0].row;
    return &medit->focused_view.file->lines.items[cursor_row];
}

void meditor_erase_line(Meditor* medit)
{
    const int cursor_row = medit->cursor_pos[0].row;
    Lines* lines = &medit->focused_view.file->lines;

    if (lines->count > 1) {
        if (cursor_row + 1 == lines->count) {
            meditor_cursor_up(medit, 1);
        }
        // Empty the erased line
        Line* erased = &lines->items[cursor_row];
        dynarray_free(*erased);
        // Shift up all lines under cursor by one
        for (int i = cursor_row; i < lines->count - cursor_row; ++i) {
            lines->items[i] = lines->items[i + 1];
        }
        // Empty the last line
        Line* last = &dynarray_last(lines);
        *last = (Line) { 0 };

        lines->count--;
    } else {
        // Empty the only line
        Line* lonely_line = &dynarray_last(lines);
        memset(lonely_line->items, '\0', lonely_line->count);
        lonely_line->count = 0;
        fixup_cursor_col(medit);
    }
}

void meditor_erase_char(Meditor* medit)
{
    const int cursor_col = medit->cursor_pos[0].col;
    const int cursor_row = medit->cursor_pos[0].row;
    Line* current_line = meditor_get_current_line(medit);

    if (cursor_col == 0) {

    } else {
        memmove(
            current_line->items + cursor_col - 1,
            current_line->items + cursor_col,
            current_line->count - cursor_col);
        dynarray_resize(current_line, current_line->count - 1);
        meditor_cursor_left(medit, 0);
    }
}
