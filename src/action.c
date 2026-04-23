#include "action.h"

#include "meditor.h"
#include "utils.h"

void medit_action_quit(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit->running = false;
}

void medit_action_cursor_up(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_up(medit);
}

void medit_action_cursor_down(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_down(medit);
}

void medit_action_cursor_left(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_left(medit);
}

void medit_action_cursor_right(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_right(medit);
}

void medit_action_restore_cursor(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    FileView* file_view = medit_get_focused_file_view(medit);
    file_view->cursors.count = 1;
    // TODO memset other cursors
}

void medit_action_cursor_line_begin(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_line_begin(medit);
}

void medit_action_cursor_line_end(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_line_end(medit);
}

void medit_action_cursor_file_begin(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_file_begin(medit);
}

void medit_action_cursor_file_end(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_cursor_file_end(medit);
}

void medit_action_add_cursor_down(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(medit);
    MEDIT_UNUSED(ui);
    // Vec2* prev_cursor = &medit->cursor_pos[medit->cursor_index];
    // Vec2* new_cursor = &medit->cursor_pos[++medit->cursor_index];
    // *new_cursor = vec2(prev_cursor->x, prev_cursor->y + 1);
}

void medit_action_erase_line(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_erase_line(medit);
}

void medit_action_focus_file_view_group_left(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    size_t* focused = &medit->file_views.focused;
    if (*focused > 0) {
        *focused -= 1;
    }
}

void medit_action_focus_file_view_group_right(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    size_t* focused = &medit->file_views.focused;
    if (*focused < medit->file_views.count - 1) {
        *focused += 1;
    }
}

void medit_action_display_file_view_left(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_file_view_group_display_prev(medit, medit_get_focused_file_view_group(medit));
}

void medit_action_display_file_view_right(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_file_view_group_display_next(medit, medit_get_focused_file_view_group(medit));
}

void medit_action_font_zoom_in(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit->config.editor_font_size = clamp_font_size(medit->config.editor_font_size + 2);
}

void medit_action_font_zoom_out(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit->config.editor_font_size = clamp_font_size(medit->config.editor_font_size - 2);
}

void medit_action_font_zoom_default(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit->config.editor_font_size = clamp_font_size(FONT_SIZE_DEFAULT);
}
