#include "meditor.h"
#include "action.h"
#include "assert.h"
#include "dynarray.h"
#include "unicode.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

#define MEDIT_LINE_DEFAULT_CAPACITY 1024

int clamp_font_size(int size)
{
    if (size > FONT_SIZE_MAX) {
        size = FONT_SIZE_MAX;
    }
    if (size < FONT_SIZE_MIN) {
        size = FONT_SIZE_MIN;
    }
    return size;
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
    cursor->grapheme_col = grapheme_col_from_byte(line->items, line->count, cursor->byte);
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
    size_t line_count = medit_file_view_file(medit, file_view)->lines.count;
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
        cursor->grapheme_col -= 1;
        cursor->len = out.len;
    } else if (cursor->line > 0) {
        --cursor->line;
        Line* upper_line = medit_get_current_line(medit);
        cursor->byte = upper_line->count;
        cursor->grapheme_col = grapheme_col_from_byte(line->items, line->count, cursor->byte);
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
        ++cursor->grapheme_col;
        // Save the length of the grapheme at the new cursor position
        uc_grapheme_iter_next(&it, &out);
        cursor->len = out.len;
    } else if (cursor->line < medit_file_view_file(medit, file_view)->lines.count - 1) {
        // End of current line, switch to the following one if any
        ++cursor->line;
        cursor->byte = 0;
        cursor->grapheme_col = 0;
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
    cursor->grapheme_col = 0;
    cursor->preferred_col = 0;
    update_cursor_len(medit);
}

void medit_cursor_line_end(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    Cursor* cursor = &file_view->cursors.items[0];

    cursor->byte = line->count;
    cursor->grapheme_col = grapheme_col_from_byte(line->items, line->count, cursor->byte);
    update_cursor_len(medit);
    update_preferred_col(medit);
}

void medit_cursor_file_begin(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Cursor* cursor = &file_view->cursors.items[0];
    cursor->line = 0;
    medit_cursor_line_begin(medit);
}

void medit_cursor_file_end(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Cursor* cursor = &file_view->cursors.items[0];
    cursor->line = medit_file_view_file(medit, file_view)->lines.count - 1;
    medit_cursor_line_end(medit);
}

void medit_split_line_at_cursor(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_line = file_view->cursors.items[0].line;
    const size_t cursor_byte = file_view->cursors.items[0].byte;

    Line* current_line = medit_get_current_line(medit);
    Line* new_line = medit_new_line_at(medit, cursor_line + 1);

    medit_file_view_file(medit, file_view)->dirty = true;

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

    medit_file_view_file(medit, file_view)->dirty = true;

    Line* current_line = medit_get_current_line(medit);

    dynarray_insert_many(current_line, text, n, cursor_col);
}

void medit_new_empty_file(Meditor* medit, FileViewGroup* group)
{
    File new_file = { 0 };
    dynarray_append(&medit->opened_files, new_file);

    FileView new_file_view = {
        .file_index = medit->opened_files.count - 1,
    };
    dynarray_append(&new_file_view.cursors, (Cursor) { 0 });

    dynarray_append(group, new_file_view);
    group->displayed = group->count - 1;

    medit_new_line_at(medit, 0);
}

File* medit_file_view_file(Meditor* medit, FileView* fv)
{
    assert(fv->file_index < medit->opened_files.count);
    return &medit->opened_files.items[fv->file_index];
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
    File new_file = { 0 };
    dynarray_append(&medit->opened_files, new_file);
    File* file = &dynarray_last(&medit->opened_files);
    file->name = medit_strdup(filepath);

    FILE* f = fopen(filepath, "r");
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
        bool last_line_ended_with_newline = false;

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
                last_line_ended_with_newline = true;
            } else {
                // Buffer was too small; accumulate into the same line
                dynarray_append_many(&current_line, buf, len);
                last_line_ended_with_newline = false;
            }
        }
        // Trailing line with no newline, or empty file
        if (current_line.count > 0 || file->lines.count == 0) {
            dynarray_append(&file->lines, current_line);
        } else if (last_line_ended_with_newline) {
            // Add empty line after final newline
            dynarray_append(&file->lines, current_line);
        } else {
            dynarray_free(current_line);
        }

        (void)fclose(f);
    }

    // Create a file view pointing to the loaded file
    FileView new_file_view = {
        .file_index = medit->opened_files.count - 1,
    };

    // Add the cursor to the top character
    Cursor cursor = { 0 };
    Line* initial_line = &file->lines.items[0];
    UcGraphemeIter it = { 0 };
    uc_grapheme_iter_init(&it, (uint8_t*)initial_line->items, initial_line->count, cursor.byte);
    UcSpan out = { 0 };
    uc_grapheme_iter_next(&it, &out);
    cursor.len = MEDIT_MAX(out.len, 1);
    dynarray_append(&new_file_view.cursors, cursor);

    dynarray_append(group, new_file_view);
    group->displayed = group->count - 1;

    printf("file lines: %zu\n", file->lines.count);
}

void medit_save_file(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    if (file_view == NULL) {
        printf("Error: no file to save\n");
        return;
    }

    const char* filepath = medit_file_view_file(medit, file_view)->name;
    if (filepath == NULL) {
        printf("Error: file has no name\n");
        return;
    }

    FILE* f = fopen(filepath, "w");
    if (f == NULL) {
        printf("Error: cannot open file %s for writing\n", filepath);
        return;
    }

    Lines* lines = &medit_file_view_file(medit, file_view)->lines;
    for (size_t i = 0; i < lines->count; ++i) {
        Line* line = &lines->items[i];
        if (line->count > 0) {
            if (fwrite(line->items, 1, line->count, f) != line->count) {
                printf("Error: failed to write to file %s\n", filepath);
                (void)fclose(f);
                return;
            }
        }
        // Write newline after each line except the last
        if (i < lines->count - 1) {
            if (fputc('\n', f) == EOF) {
                printf("Error: failed to write newline to file %s\n", filepath);
                (void)fclose(f);
                return;
            }
        }
    }

    (void)fclose(f);
    printf("File saved: %s\n", filepath);

    medit_file_view_file(medit, file_view)->dirty = false;
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
    Lines* lines = &medit_file_view_file(medit, file_view)->lines;

    medit_file_view_file(medit, file_view)->dirty = true;

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
    return &medit_file_view_file(medit, file_view)->lines.items[cursor_row];
}

void medit_erase_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_row = file_view->cursors.items[0].line;
    Lines* lines = &medit_file_view_file(medit, file_view)->lines;

    medit_file_view_file(medit, file_view)->dirty = true;

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

    Lines* lines = &medit_file_view_file(medit, file_view)->lines;
    Line* current_line = &lines->items[cursor_line];

    medit_file_view_file(medit, file_view)->dirty = true;

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
    }

    // Update the length of the cursor as the character under it changed
    update_cursor_len(medit);
}

void medit_layout_recompute(Layout* layout, FileViewGroups* groups, size_t win_w, size_t win_h)
{
    assert(groups->count > 0 && "You forgot to create some demo group");

    LayoutSizes s = layout->sizes;
    Rect window = { 0, 0, win_w, win_h };

    if (medit_layout_is_element_shown(layout, LAYOUT_MENU_BAR)) {
        layout->menu_bar = panel_cut_top(&window, s.menu_bar_height, s.separator_size);
    }
    if (medit_layout_is_element_shown(layout, LAYOUT_STATUS_BAR)) {
        layout->status_bar = panel_cut_bottom(&window, s.status_bar_height, s.separator_size);
    }
    if (medit_layout_is_element_shown(layout, LAYOUT_SIDE_PANEL)) {
        layout->side_panel = panel_cut_left(&window, s.side_panel_width, s.separator_size);
    }
    if (medit_layout_is_element_shown(layout, LAYOUT_BOTTOM_PANEL)) {
        layout->bottom_panel = panel_cut_bottom(&window, s.bottom_panel_height, s.separator_size);
    }
    layout->editor_area = window;

    size_t cols = 1;
    while (cols * cols < groups->count) {
        cols++;
    }
    size_t rows = (groups->count + cols - 1) / cols;
    size_t row_height = window.h / rows;
    size_t col_width = window.w / cols;

    Rect editor = layout->editor_area;

    for (size_t r = 0; r < rows; ++r) {
        if (r > 0) {
            rect_cut_top(&editor, s.separator_size);
        }
        Rect row_rect = rect_cut_top(&editor, row_height);
        for (size_t c = 0; c < cols; ++c) {
            if (c > 0) {
                rect_cut_left(&row_rect, s.separator_size);
            }
            size_t idx = (r * cols) + c;
            if (idx >= groups->count) {
                break;
            }
            groups->items[idx].area = rect_cut_left(&row_rect, col_width);
            Rect content = groups->items[idx].area;
            if (medit_layout_is_element_shown(layout, LAYOUT_TAB_BAR)) {
                rect_cut_top(&content, s.tab_bar_height + s.separator_size);
            }
            groups->items[idx].content_area = content;
        }
    }
}

void medit_dump_state(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    printf("Dump state:\n");
    printf(
        "  cursor: byte=%zu, line=%zu; lines:%zu\n  lines:\n",
        file_view->cursors.items[0].byte,
        file_view->cursors.items[0].line,
        medit_file_view_file(medit, file_view)->lines.count);
    Lines* lines = &medit_file_view_file(medit, file_view)->lines;
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

void medit_load_default_keybind_full(Meditor* medit, const Actions* actions, void* ui)
{
    Keybind* keybind = &medit->keybind;

    keybind_bind(keybind, KEY_Q, MOD_CTRL, actions->quit, medit, ui);
    keybind_bind(keybind, KEY_S, MOD_CTRL, actions->save_file, medit, ui);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, actions->font_zoom_in, medit, ui);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, actions->font_zoom_in, medit, ui);
    keybind_bind(keybind, KEY_NPAD_MINUS, MOD_CTRL, actions->font_zoom_out, medit, ui);
    keybind_bind(keybind, KEY_6, MOD_CTRL, actions->font_zoom_out, medit, ui);
    keybind_bind(keybind, KEY_EQUALS, MOD_CTRL, actions->font_zoom_default, medit, ui);

    keybind_bind(keybind, KEY_UP, MOD_NONE, actions->cursor_up, medit, ui);
    keybind_bind(keybind, KEY_DOWN, MOD_NONE, actions->cursor_down, medit, ui);
    keybind_bind(keybind, KEY_LEFT, MOD_NONE, actions->cursor_left, medit, ui);
    keybind_bind(keybind, KEY_RIGHT, MOD_NONE, actions->cursor_right, medit, ui);
    keybind_bind(keybind, KEY_HOME, MOD_NONE, actions->cursor_line_begin, medit, ui);
    keybind_bind(keybind, KEY_END, MOD_NONE, actions->cursor_line_end, medit, ui);
    keybind_bind(keybind, KEY_HOME, MOD_CTRL, actions->cursor_file_begin, medit, ui);
    keybind_bind(keybind, KEY_END, MOD_CTRL, actions->cursor_file_end, medit, ui);

    keybind_bind(keybind, KEY_ESCAPE, MOD_NONE, actions->restore_cursor, medit, ui);
    keybind_bind(keybind, KEY_DOWN, MOD_CTRL_ALT, actions->add_cursor_down, medit, ui);

    keybind_bind(keybind, KEY_D, MOD_CTRL, actions->dump_state, medit, ui);

    keybind_bind(keybind, KEY_K, MOD_SHIFT_CTRL, actions->erase_line, medit, ui);

    keybind_bind(keybind, KEY_LEFT, MOD_ALT, actions->focus_file_view_group_left, medit, ui);
    keybind_bind(keybind, KEY_RIGHT, MOD_ALT, actions->focus_file_view_group_right, medit, ui);

    keybind_bind(keybind, KEY_O, MOD_CTRL, actions->open_file_dialog, medit, ui);

    keybind_bind(keybind, KEY_B, MOD_CTRL, actions->toggle_side_panel, medit, ui);
}
