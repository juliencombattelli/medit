#include "meditor.h"
#include "dynarray.h"
#include "unicode.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

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

static void action_font_zoom_out(Meditor* medit)
{
    set_font_size_clamped(&medit->config.editor_font_size, medit->config.editor_font_size - 2);
}

static void action_font_zoom_in(Meditor* medit)
{
    set_font_size_clamped(&medit->config.editor_font_size, medit->config.editor_font_size + 2);
}

static void action_font_zoom_default(Meditor* medit)
{
    set_font_size_clamped(&medit->config.editor_font_size, FONT_SIZE_DEFAULT);
}

static void action_cursor_up(Meditor* medit)
{
    medit_cursor_up(medit);
}

static void action_cursor_down(Meditor* medit)
{
    medit_cursor_down(medit);
}

static void action_cursor_left(Meditor* medit)
{
    medit_cursor_left(medit);
}

static void action_cursor_right(Meditor* medit)
{
    medit_cursor_right(medit);
}

static void action_restore_cursor(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    file_view->cursors.count = 1;
    // TODO memset other cursors
}

static void action_cursor_line_begin(Meditor* medit)
{
    medit_cursor_line_begin(medit);
}

static void action_cursor_line_end(Meditor* medit)
{
    medit_cursor_line_end(medit);
}

static void action_add_cursor_down(Meditor* medit)
{
    MEDIT_UNUSED(medit);
    // Vec2* prev_cursor = &medit->cursor_pos[medit->cursor_index];
    // Vec2* new_cursor = &medit->cursor_pos[++medit->cursor_index];
    // *new_cursor = vec2(prev_cursor->x, prev_cursor->y + 1);
}

static void action_dump_state(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    printf("Dump state:\n");
    printf(
        "  cursor: byte=%zu, line=%zu, x=%zu, y=%zu, w=%zu, h=%zu; lines:%zu\n  lines:\n",
        file_view->cursors.items[0].byte,
        file_view->cursors.items[0].line,
        file_view->cursors.items[0].on_screen.x,
        file_view->cursors.items[0].on_screen.y,
        file_view->cursors.items[0].on_screen.w,
        file_view->cursors.items[0].on_screen.h,
        file_view->file->lines.count);
    Lines* lines = &file_view->file->lines;
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
    medit_erase_line(medit);
}

static void action_focus_file_view_group_left(Meditor* medit)
{
    size_t* focused = &medit->file_views.focused;
    if (*focused > 0) {
        *focused -= 1;
    }
}

static void action_focus_file_view_group_right(Meditor* medit)
{
    size_t* focused = &medit->file_views.focused;
    if (*focused < medit->file_views.count - 1) {
        *focused += 1;
    }
}

void medit_load_default_gui_keybind(Meditor* medit)
{
    Keybind* keybind = &medit->keybind;

    keybind_bind(keybind, KEY_Q, MOD_CTRL, action_quit, medit);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, action_font_zoom_in, medit);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, action_font_zoom_in, medit);
    keybind_bind(keybind, KEY_EQUALS, MOD_CTRL, action_font_zoom_default, medit);

    keybind_bind(keybind, KEY_NPAD_MINUS, MOD_CTRL, action_font_zoom_out, medit);
    keybind_bind(keybind, KEY_6, MOD_CTRL, action_font_zoom_out, medit);

    keybind_bind(keybind, KEY_UP, MOD_NONE, action_cursor_up, medit);
    keybind_bind(keybind, KEY_DOWN, MOD_NONE, action_cursor_down, medit);
    keybind_bind(keybind, KEY_LEFT, MOD_NONE, action_cursor_left, medit);
    keybind_bind(keybind, KEY_RIGHT, MOD_NONE, action_cursor_right, medit);
    keybind_bind(keybind, KEY_HOME, MOD_NONE, action_cursor_line_begin, medit);
    keybind_bind(keybind, KEY_END, MOD_NONE, action_cursor_line_end, medit);

    keybind_bind(keybind, KEY_ESCAPE, MOD_NONE, action_restore_cursor, medit);
    keybind_bind(keybind, KEY_DOWN, MOD_CTRL_ALT, action_add_cursor_down, medit);

    keybind_bind(keybind, KEY_D, MOD_CTRL, action_dump_state, medit);

    keybind_bind(keybind, KEY_K, MOD_SHIFT_CTRL, action_erase_line, medit);

    keybind_bind(keybind, KEY_LEFT, MOD_ALT, action_focus_file_view_group_left, medit);
    keybind_bind(keybind, KEY_RIGHT, MOD_ALT, action_focus_file_view_group_right, medit);
}

void medit_load_default_tui_keybind(Meditor* medit)
{
    MEDIT_UNUSED(medit);
}

// Update the length of the grapheme at the cursor position
static void update_cursor_len(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    Cursor* cursor = &file_view->cursors.items[0];

    UcGraphemeIter it = { 0 };
    uc_grapheme_iter_init(&it, (uint8_t*)line->items, line->count, cursor->byte);
    UcSpan out = { 0 };
    uc_grapheme_iter_next(&it, &out);
    cursor->len = MEDIT_MAX(out.len, 1);
}

// Count the number of grapheme clusters from byte 0 up to `byte` in `line[0..len)`.
static size_t grapheme_col_from_byte(const char* line, size_t len, size_t byte)
{
    UcGraphemeIter it;
    uc_grapheme_iter_init(&it, (uint8_t*)line, len, 0);
    size_t col = 0;
    UcSpan span;
    while (it.pos < byte && uc_grapheme_iter_next(&it, &span)) {
        ++col;
    }
    return col;
}

// Return the byte offset of the `col`-th grapheme cluster start in `line[0..len)`.
// If the line has fewer than `col` graphemes, returns `len` (end of line).
static size_t byte_from_grapheme_col(const char* line, size_t len, size_t col)
{
    UcGraphemeIter it;
    uc_grapheme_iter_init(&it, (uint8_t*)line, len, 0);
    size_t c = 0;
    UcSpan span;
    while (c < col && uc_grapheme_iter_next(&it, &span)) {
        ++c;
    }
    return it.pos;
}

// Update preferred_col to reflect the cursor's current byte position on the current line.
// Call this after every horizontal cursor move.
static void update_preferred_col(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    Cursor* cursor = &file_view->cursors.items[0];
    cursor->preferred_col = grapheme_col_from_byte(line->items, line->count, cursor->byte);
}

// Adjust cursor column position mainly when switching line
static void fixup_cursor_col(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    Cursor* cursor = &file_view->cursors.items[0];

    cursor->byte = byte_from_grapheme_col(line->items, line->count, cursor->preferred_col);
    update_cursor_len(medit);
}

void medit_cursor_up(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Cursor* cursor = &file_view->cursors.items[0];
    if (cursor->line > 0) {
        --cursor->line;
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

void medit_cursor_down(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    size_t line_count = file_view->file->lines.count;
    Cursor* cursor = &file_view->cursors.items[0];
    if (cursor->line < line_count - 1) {
        ++cursor->line;
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

void medit_cursor_left(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    Cursor* cursor = &file_view->cursors.items[0];
    if (cursor->byte > 0) {
        UcGraphemeIter it = { 0 };
        uc_grapheme_iter_init(&it, (uint8_t*)line->items, line->count, cursor->byte);
        UcSpan out = { 0 };
        // Move the cursor to the previous grapheme by shifting left by the length of the current
        // grapheme and save its length
        uc_grapheme_iter_prev(&it, &out);
        cursor->byte -= out.len;
        cursor->len = out.len;
    } else if (cursor->line > 0) {
        --cursor->line;
        Line* upper_line = medit_get_current_line(medit);
        cursor->byte = upper_line->count;
        update_cursor_len(medit);
    }
    update_preferred_col(medit);
    // TODO handle multi cursor
    // for (size_t c = 0; c <= medit->cursor_index; ++c) {
    //     Vec2* cursor = &medit->cursor_pos[c];
    //     cursor->col -= cells;
    //     if (cursor->col < 0) {
    //         cursor->col = 0;
    //     }
    // }
}

void medit_cursor_right(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    Cursor* cursor = &file_view->cursors.items[0];
    if (cursor->byte < line->count) {
        UcGraphemeIter it = { 0 };
        uc_grapheme_iter_init(&it, (uint8_t*)line->items, line->count, cursor->byte);
        UcSpan out = { 0 };
        // Move the cursor to the next grapheme by shifting right by the length of the current
        // grapheme
        uc_grapheme_iter_next(&it, &out);
        cursor->byte += out.len;
        // Save the length of the grapheme at the new cursor position
        uc_grapheme_iter_next(&it, &out);
        cursor->len = out.len;
    } else if (cursor->line < file_view->file->lines.count - 1) {
        // End of current line, switch to the following one if any
        ++cursor->line;
        cursor->byte = 0;
        update_cursor_len(medit);
    }
    update_preferred_col(medit);
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

void medit_cursor_line_begin(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Cursor* cursor = &file_view->cursors.items[0];

    cursor->byte = 0;
    cursor->preferred_col = 0;
    update_cursor_len(medit);
}

void medit_cursor_line_end(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);

    file_view->cursors.items[0].byte = line->count;
    update_cursor_len(medit);
    update_preferred_col(medit);
}

void medit_split_line_at_cursor(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_line = file_view->cursors.items[0].line;
    const size_t cursor_byte = file_view->cursors.items[0].byte;

    Line* current_line = medit_get_current_line(medit);
    Line* new_line = medit_new_line_at(medit, cursor_line + 1);

    dynarray_insert_many(
        new_line,
        &current_line->items[cursor_byte],
        current_line->count - cursor_byte,
        0);
    memset(&current_line->items[cursor_byte], 0, current_line->count - cursor_byte);
    current_line->count = cursor_byte;
}

void medit_insert_text(Meditor* medit, const char* text, size_t n)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_col = file_view->cursors.items[0].byte;

    Line* current_line = medit_get_current_line(medit);

    dynarray_insert_many(current_line, text, n, cursor_col);
}

void medit_new_empty_file(Meditor* medit, FileViewGroup* group)
{
    File new_file = { 0 };
    dynarray_append(&medit->opened_files, new_file);

    FileView new_file_view = {
        .file = &dynarray_last(&medit->opened_files),
    };
    dynarray_append(&new_file_view.cursors, (Cursor) { 0 });

    dynarray_append(group, new_file_view);

    medit_new_line_at(medit, 0);
}

void medit_load_file(Meditor* medit, const char* filepath)
{
    // Create a group if none exists
    if (medit->file_views.count == 0) {
        dynarray_append(&medit->file_views, (FileViewGroup) { 0 });
        medit->file_views.focused = 0;
    }
    FileViewGroup* group = medit_get_focused_file_view_group(medit);

    // Create and register the new file
    File new_file = { .name = filepath };
    dynarray_append(&medit->opened_files, new_file);
    File* file = &dynarray_last(&medit->opened_files);

    FILE* f = fopen(filepath, "re");
    if (f == NULL) {
        printf("Error: cannot open file %s\n", filepath);
        // Fall back to a single empty line
        Line empty_line = { 0 };
        dynarray_reserve(&empty_line, MEDIT_LINE_DEFAULT_CAPACITY);
        dynarray_append(&file->lines, empty_line);
    } else {
        char buf[MEDIT_LINE_DEFAULT_CAPACITY];
        Line current_line = { 0 };
        dynarray_reserve(&current_line, MEDIT_LINE_DEFAULT_CAPACITY);

        // TODO the last empty line of a file is not loaded somehow
        while (fgets(buf, sizeof(buf), f) != NULL) {
            size_t len = strlen(buf);
            // Strip trailing CRLF or LF
            if (len > 0 && buf[len - 1] == '\n') {
                len--;
                if (len > 0 && buf[len - 1] == '\r') {
                    len--;
                }
                dynarray_append_many(&current_line, buf, len);
                dynarray_append(&file->lines, current_line);
                current_line = (Line) { 0 };
                dynarray_reserve(&current_line, MEDIT_LINE_DEFAULT_CAPACITY);
            } else {
                // Buffer was too small; accumulate into the same line
                dynarray_append_many(&current_line, buf, len);
            }
        }
        // Trailing line with no newline, or empty file
        if (current_line.count > 0 || file->lines.count == 0) {
            dynarray_append(&file->lines, current_line);
        } else {
            dynarray_free(current_line);
        }

        (void)fclose(f);
    }

    // Create a file view pointing to the loaded file
    FileView new_file_view = {
        .file = file,
    };
    dynarray_append(&new_file_view.cursors, (Cursor) { 0 });

    dynarray_append(group, new_file_view);
    group->displayed = group->count - 1;
}

void medit_close_files(Meditor* medit)
{
    dynarray_foreach(FileViewGroup, group, &medit->file_views)
    {
        dynarray_foreach(FileView, fv, group)
        {
            dynarray_free(fv->cursors);
        }
        dynarray_free(*group);
    }
    dynarray_free(medit->file_views);

    dynarray_foreach(File, file, &medit->opened_files)
    {
        dynarray_foreach(Line, line, &file->lines)
        {
            dynarray_free(*line);
        }
        dynarray_free(file->lines);
    }
    dynarray_free(medit->opened_files);
}

Line* medit_new_line_at(Meditor* medit, size_t pos)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Lines* lines = &file_view->file->lines;

    Line empty_line = { 0 };
    dynarray_reserve(&empty_line, MEDIT_LINE_DEFAULT_CAPACITY);

    dynarray_insert(lines, empty_line, pos);

    return &lines->items[pos];
}

FileViewGroup* medit_get_focused_file_view_group(Meditor* medit)
{
    assert(medit->file_views.items != NULL);
    assert(medit->file_views.focused < medit->file_views.count);
    return &medit->file_views.items[medit->file_views.focused];
}

FileView* medit_get_displayed_file_view_in_group(Meditor* medit, FileViewGroup* group)
{
    MEDIT_UNUSED(medit);
    assert(group->displayed < group->count);
    return &group->items[group->displayed];
}

FileView* medit_get_focused_file_view(Meditor* medit)
{
    FileViewGroup* group = medit_get_focused_file_view_group(medit);
    return medit_get_displayed_file_view_in_group(medit, group);
}

Line* medit_get_current_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    const size_t cursor_row = file_view->cursors.items[0].line;
    return &file_view->file->lines.items[cursor_row];
}

void medit_erase_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_row = file_view->cursors.items[0].line;
    Lines* lines = &file_view->file->lines;

    if (lines->count > 1) {
        if (cursor_row + 1 == lines->count) {
            medit_cursor_up(medit);
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

void medit_erase_char(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_byte = file_view->cursors.items[0].byte;
    const size_t cursor_line = file_view->cursors.items[0].line;

    if (cursor_byte == 0 && cursor_line == 0) {
        return;
    }

    Lines* lines = &file_view->file->lines;
    Line* current_line = &lines->items[cursor_line];

    medit_cursor_left(medit);

    if (cursor_byte == 0) {
        // Merge the current line with the upper one
        Line* upper_line = &lines->items[cursor_line - 1];
        dynarray_append_many(upper_line, current_line->items, current_line->count);
        // Remove the current line
        Line erased = lines->items[cursor_line];
        dynarray_free(erased);
        dynarray_remove(lines, cursor_line);
    } else {
        Cursor* cursor = &file_view->cursors.items[0];
        // Remove the grapheme before the cursor
        dynarray_remove_many(current_line, cursor_byte - cursor->len, cursor->len);
        // Update the length of the cursor as the character under it was removed and the whole line
        // shifted left by one grapheme
        update_cursor_len(medit);
    }
}
