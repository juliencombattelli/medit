#include "meditor.h"
#include "dynarray.h"
#include "utf8.h"

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

static void action_cursor_begin_of_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    file_view->cursors.items[0].in_file.byte = 0;
}

static void action_cursor_end_of_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    file_view->cursors.items[0].in_file.byte = line->count;
}

static void action_add_cursor_down(Meditor* medit)
{
    // Vec2* prev_cursor = &medit->cursor_pos[medit->cursor_index];
    // Vec2* new_cursor = &medit->cursor_pos[++medit->cursor_index];
    // *new_cursor = vec2(prev_cursor->x, prev_cursor->y + 1);
}

static void action_dump_state(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    printf("Dump state:\n");
    printf(
        "  cursor: byte=%zu, line=%zu; lines:%zu\n  lines:\n",
        file_view->cursors.items[0].in_file.byte,
        file_view->cursors.items[0].in_file.line,
        file_view->file->lines.count);
    Lines* lines = &file_view->file->lines;
    int row = 0;
    dynarray_foreach(Line, line, lines)
    {
        if (line->count != 0) {
            printf(
                "    #%d:`%.*s` (%zu bytes) \n",
                row++,
                (int)line->count,
                line->items,
                line->count);
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

    keybind_bind(keybind, KEY_A, MOD_CTRL, action_toggle_debug_grid, medit);

    keybind_bind(keybind, KEY_NPAD_PLUS, MOD_CTRL, action_font_zoom_in, medit);
    keybind_bind(keybind, KEY_EQUALS, MOD_SHIFT_CTRL, action_font_zoom_in, medit);
    keybind_bind(keybind, KEY_EQUALS, MOD_CTRL, action_font_zoom_default, medit);

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

    keybind_bind(keybind, KEY_LEFT, MOD_ALT, action_focus_file_view_group_left, medit);
    keybind_bind(keybind, KEY_RIGHT, MOD_ALT, action_focus_file_view_group_right, medit);
}

void medit_load_default_tui_keybind(Meditor* medit)
{
    (void)medit;
}

static void fixup_cursor_col(Meditor* medit)
{
    // Clamp in_file.byte to the new line length and snap to a codepoint boundary
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    FileCoord* pos = &file_view->cursors.items[0].in_file;
    if (pos->byte > line->count) {
        pos->byte = line->count;
    }
    // Walk back until we are not at a UTF-8 continuation byte
    while (pos->byte > 0 && ((unsigned char)line->items[pos->byte] & 0xC0) == 0x80) {
        --pos->byte;
    }
}

void medit_cursor_up(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    FileCoord* pos = &file_view->cursors.items[0].in_file;
    if (pos->line > 0) {
        --pos->line;
    }
    fixup_cursor_col(medit);
}

void medit_cursor_down(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    FileCoord* pos = &file_view->cursors.items[0].in_file;
    if (pos->line < file_view->file->lines.count - 1) {
        ++pos->line;
    }
    fixup_cursor_col(medit);
}

void medit_cursor_left(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    FileCoord* pos = &file_view->cursors.items[0].in_file;
    if (pos->byte > 0) {
        Line* line = medit_get_current_line(medit);
        pos->byte -= 1; // utf8_prev_codepoint_size(line->items, pos->byte);
    } else if (pos->line > 0) {
        --pos->line;
        Line* upper_line = medit_get_current_line(medit);
        pos->byte = upper_line->count;
    }
}

void medit_cursor_right(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);
    Line* line = medit_get_current_line(medit);
    FileCoord* pos = &file_view->cursors.items[0].in_file;
    if (pos->byte < line->count) {
        printf("pos->byte preincr: %d\n", pos->byte);
        pos->byte += 1; // utf8_codepoint_size((unsigned char)line->items[pos->byte]);
        printf("pos->byte postincr: %d\n", pos->byte);
    } else if (pos->line < file_view->file->lines.count - 1) {
        ++pos->line;
        pos->byte = 0;
    }
}

void medit_split_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_byte = file_view->cursors.items[0].in_file.byte;
    const size_t cursor_line = file_view->cursors.items[0].in_file.line;
    medit_insert_new_line(medit);
    medit_cursor_down(medit);
    Line* current_line = &file_view->file->lines.items[cursor_line];
    if (current_line->count != 0) {
        medit_insert_text(
            medit,
            &current_line->items[cursor_byte],
            current_line->count - cursor_byte,
            0);
        memset(&current_line->items[cursor_byte], 0, current_line->count - cursor_byte);
        current_line->count = cursor_byte;
    }
}

void medit_insert_text(Meditor* medit, const char* text, size_t n, size_t cells)
{
    (void)cells;
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_byte = file_view->cursors.items[0].in_file.byte;

    Line* current_line = medit_get_current_line(medit);

    dynarray_insert_many(current_line, text, n, cursor_byte);
}

void medit_new_empty_file(Meditor* medit)
{
    File new_file = { 0 };
    dynarray_append(&medit->opened_files, new_file);

    FileView new_file_view = {
        .file = &dynarray_last(&medit->opened_files),
    };
    dynarray_append(&new_file_view.cursors, (Cursor) { 0 });

    FileViewGroup new_file_view_group = { 0 };
    dynarray_append(&new_file_view_group, new_file_view);

    medit->file_views.focused = medit->file_views.count;
    dynarray_append(&medit->file_views, new_file_view_group);

    medit_insert_new_line(medit);
}

void medit_close_files(Meditor* medit)
{
    dynarray_foreach(FileViewGroup, fvgroup, &medit->file_views)
    {
        dynarray_foreach(FileView, fv, fvgroup)
        {
            dynarray_free(fv->cursors);
        }
        dynarray_free(*fvgroup);
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

void medit_insert_new_line(Meditor* medit)
{
    // TODO verify if next line is empty with a non-null capacity for reuse
    FileView* file_view = medit_get_focused_file_view(medit);

    Lines* lines = &file_view->file->lines;

    Line empty_line = { 0 };
    dynarray_reserve(&empty_line, MEDIT_LINE_DEFAULT_CAPACITY);

    const size_t line_pos = lines->count > 0 ? file_view->cursors.items[0].in_file.line + 1 : 0;

    dynarray_insert(lines, empty_line, line_pos);
}

FileViewGroup* medit_get_focused_file_view_group(Meditor* medit)
{
    assert(medit->file_views.items != NULL);
    assert(medit->file_views.focused < medit->file_views.count);
    return &medit->file_views.items[medit->file_views.focused];
}

FileView* medit_get_displayed_file_view_in_group(Meditor* medit, FileViewGroup* group)
{
    (void)medit;
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
    const size_t cursor_line = file_view->cursors.items[0].in_file.line;
    return &file_view->file->lines.items[cursor_line];
}

void medit_erase_line(Meditor* medit)
{
    FileView* file_view = medit_get_focused_file_view(medit);

    const size_t cursor_row = file_view->cursors.items[0].in_file.line;
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

    const size_t cursor_byte = file_view->cursors.items[0].in_file.byte;
    const size_t cursor_line = file_view->cursors.items[0].in_file.line;

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
        dynarray_remove(current_line, cursor_byte - 1);
        // // Remove the full UTF-8 codepoint immediately before the cursor
        // size_t cp_size = utf8_prev_codepoint_size(current_line->items, cursor_byte);
        // for (size_t i = 0; i < cp_size; ++i) {
        //     dynarray_remove(current_line, cursor_byte - cp_size);
        // }
    }
}
