#include "meditor.h"
#include "dynarray.h"

#include <string.h>

#define MEDIT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MEDIT_MAX(a, b) ((a) > (b) ? (a) : (b))

#define MEDIT_LINE_DEFAULT_CAPACITY 1024

static void set_font_size_clamped(int* font, int value)
{
    if (value > FONT_SIZE_MAX) {
        value = FONT_SIZE_MAX;
    }
    if (value < FONT_SIZE_MIN) {
        value = FONT_SIZE_MIN;
    }
    *font = value;
}

static void action_quit(Meditor* medit)
{
    medit->running = false;
}

static void action_toggle_debug_grid(Meditor* medit)
{
    medit->draw_debug_grid = !medit->draw_debug_grid;
}

static void action_font_zoom_out(Meditor* medit)
{
    set_font_size_clamped(
        &medit->startup_config.editor_font_size,
        medit->startup_config.editor_font_size - 2);
    medit_unload_font(medit);
    medit_load_font(medit);
}

static void action_font_zoom_in(Meditor* medit)
{
    set_font_size_clamped(
        &medit->startup_config.editor_font_size,
        medit->startup_config.editor_font_size + 2);
    medit_unload_font(medit);
    medit_load_font(medit);
}

static void action_cursor_up(Meditor* medit)
{
    meditor_cursor_up(medit, 1);
}

static void action_cursor_down(Meditor* medit)
{
    meditor_cursor_down(medit, 1);
}

static void action_cursor_left(Meditor* medit)
{
    meditor_cursor_left(medit, 1);
}

static void action_cursor_right(Meditor* medit)
{
    meditor_cursor_right(medit, 1);
}

static void action_restore_cursor(Meditor* medit)
{
    medit->focused_view.cursors.count = 0;
    // TODO memset other cursors
}

static void action_cursor_begin_of_line(Meditor* medit)
{
    medit->focused_view.cursors.items[0].col = 0;
}

static void action_cursor_end_of_line(Meditor* medit)
{
    Line* line = meditor_get_current_line(medit);
    medit->focused_view.cursors.items[0].col = line->count;
}

static void action_add_cursor_down(Meditor* medit)
{
    // Vec2* prev_cursor = &medit->cursor_pos[medit->cursor_index];
    // Vec2* new_cursor = &medit->cursor_pos[++medit->cursor_index];
    // *new_cursor = vec2(prev_cursor->x, prev_cursor->y + 1);
}

static void action_dump_state(Meditor* medit)
{
    printf("Dump state:\n");
    printf(
        "  cursor: c=%zu, r=%zu; lines:%zu\n  lines:\n",
        medit->focused_view.cursors.items[0].col,
        medit->focused_view.cursors.items[0].row,
        medit->focused_view.file->lines.count);
    Lines* lines = &medit->focused_view.file->lines;
    int row = 0;
    dynarray_foreach(Line, line, lines)
    {
        if (line->count != 0) {
            printf("    #%d:`%.*s`\n", row++, (int)line->count, line->items);
        } else {
            printf("    #%d:``\n", row++);
        }
    }
}

static void action_erase_line(Meditor* medit)
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
    keybind_bind(keybind, KEY_HOME, MOD_NONE, action_cursor_begin_of_line, medit);
    keybind_bind(keybind, KEY_END, MOD_NONE, action_cursor_end_of_line, medit);

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
    Cell* cursor = &medit->focused_view.cursors.items[0];
    cursor->col = MEDIT_MIN(cursor->col, row_length);
}

void meditor_cursor_up(Meditor* medit, size_t cells)
{
    Cell* cursor = &medit->focused_view.cursors.items[0];
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

void meditor_cursor_down(Meditor* medit, size_t cells)
{
    size_t line_count = medit->focused_view.file->lines.count;
    Cell* cursor = &medit->focused_view.cursors.items[0];
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

void meditor_cursor_left(Meditor* medit, size_t cells)
{
    Cell* cursor = &medit->focused_view.cursors.items[0];
    if (cursor->col > 0) {
        --cursor->col;
    } else if (cursor->row > 0) {
        --cursor->row;
        Line* upper_line = meditor_get_current_line(medit);
        cursor->col = upper_line->count;
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

void meditor_cursor_right(Meditor* medit, size_t cells)
{
    Line* line = meditor_get_current_line(medit);
    Cell* cursor = &medit->focused_view.cursors.items[0];
    if (cursor->col < line->count) {
        ++cursor->col;
    } else if (cursor->row < medit->focused_view.file->lines.count - 1) {
        ++cursor->row;
        cursor->col = 0;
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
    const size_t cursor_col = medit->focused_view.cursors.items[0].col;
    const size_t cursor_row = medit->focused_view.cursors.items[0].row;
    meditor_insert_new_line(medit);
    meditor_cursor_down(medit, 0);
    Line* current_line = &medit->focused_view.file->lines.items[cursor_row];
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

void meditor_insert_text(Meditor* medit, const char* text, size_t n, size_t cells)
{
    const size_t cursor_col = medit->focused_view.cursors.items[0].col;

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
    dynarray_append(&new_file_view.cursors, (Cell) { 0 });
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

    const size_t line_pos = lines->count > 0 ? medit->focused_view.cursors.items[0].row + 1 : 0;

    dynarray_insert(lines, empty_line, line_pos);
}

Line* meditor_get_current_line(Meditor* medit)
{
    const size_t cursor_row = medit->focused_view.cursors.items[0].row;
    return &medit->focused_view.file->lines.items[cursor_row];
}

void meditor_erase_line(Meditor* medit)
{
    const size_t cursor_row = medit->focused_view.cursors.items[0].row;
    Lines* lines = &medit->focused_view.file->lines;

    if (lines->count > 1) {
        if (cursor_row + 1 == lines->count) {
            meditor_cursor_up(medit, 1);
        }
        Line erased = lines->items[cursor_row];
        dynarray_free(erased);
        dynarray_remove(lines, cursor_row);
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
    const size_t cursor_col = medit->focused_view.cursors.items[0].col;
    const size_t cursor_row = medit->focused_view.cursors.items[0].row;
    Line* current_line = meditor_get_current_line(medit);

    if (cursor_col == 0 && cursor_row == 0) {
        return;
    }
    if (cursor_col == 0 && cursor_row > 0) {
        size_t leftover = current_line->count;
        Line* upper_line = &medit->focused_view.file->lines.items[cursor_row - 1];
        dynarray_append_many(upper_line, current_line->items, leftover);
        meditor_erase_line(medit);
        medit->focused_view.cursors.items[0].col = upper_line->count - leftover;
    } else {
        dynarray_remove(current_line, cursor_col - 1);
        meditor_cursor_left(medit, 0);
    }
}
