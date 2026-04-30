#include "sdl3_internal.h"

#include "utils/utils.h"

#include <core/assert.h>
#include <core/dynarray.h>
#include <core/safeint.h>
#include <core/utils.h>

static Rect ui_sdl3_cursor_rect(
    SDL3Ui* ui,
    Rect text_area,
    const Cursor* cursor,
    FileView* file_view)
{
    Line* line = &medit_file_view_file(ui->medit, file_view)->lines.items[cursor->line];
    // Compute x offset of the cursor in the line
    int line_w = 0;
    if (cursor->byte != 0) {
        TTF_MeasureString(
            ui->font_editor.main,
            medit_file_view_file(ui->medit, file_view)->lines.items[cursor->line].items,
            cursor->byte,
            0,
            &line_w,
            NULL);
    }
    // Compute cursor width
    int cursor_w = size_to_int(ui->font_editor.default_cursor_width);
    if (line->count != cursor->byte) {
        // cursor not at end of line (excludes also empty lines)
        TTF_MeasureString(
            ui->font_editor.main,
            &line->items[cursor->byte],
            cursor->len,
            0,
            &cursor_w,
            NULL);
    }
    return (Rect) {
        .x = text_area.x + line_w,
        .y = text_area.y + size_to_i32(cursor->line * ui->font_editor.line_spacing),
        .w = cursor_w,
        .h = size_to_i32(ui->font_editor.line_spacing),
    };
}

// Queue cursor rect(s) into the overlay draw layer
void ui_sdl3_queue_cursor(SDL3Ui* ui, Rect text_area, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    Color cursor_color = ui->medit->config.color_theme.cursor;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    bool focused = medit_get_focused_file_view_group(medit) == group;

    if (focused && !ui->cursor_blinker.show) {
        return;
    }

    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        const Cursor* cursor = &file_view->cursors.items[i];
        const Rect on_screen = ui_sdl3_cursor_rect(ui, text_area, cursor, file_view);

        Rect cursor_rect = {
            .x = on_screen.x - file_view->scrolling.x,
            .y = on_screen.y - file_view->scrolling.y,
            .w = on_screen.w,
            .h = on_screen.h,
        };

        if (focused) {
            UiDrawCmd fill_cmd = {
                .kind = UI_CMD_RECT_FILLED,
                .rect = cursor_rect,
                .color = cursor_color,
            };
            dynarray_append(&ui->ui_draw_list_overlay, fill_cmd);
        } else {
            UiDrawCmd outline_cmd = {
                .kind = UI_CMD_RECT_OUTLINED,
                .rect = cursor_rect,
                .color = cursor_color,
                .outline_color = cursor_color,
            };
            dynarray_append(&ui->ui_draw_list_overlay, outline_cmd);
        }
    }
}

// Draw the glyph on top of each cursor rect directly
// Must be called after the overlay layer has been flushed
void ui_sdl3_draw_cursor_glyphs(SDL3Ui* ui, Rect text_area, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    Color cursor_color = ui->medit->config.color_theme.cursor;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    bool focused = medit_get_focused_file_view_group(medit) == group;

    if (focused && !ui->cursor_blinker.show) {
        return;
    }

    for (size_t i = 0; i < file_view->cursors.count; ++i) {
        const Cursor* cursor = &file_view->cursors.items[i];
        const Rect on_screen = ui_sdl3_cursor_rect(ui, text_area, cursor, file_view);

        Color glyph_color = focused ? color_inverse(cursor_color) : cursor_color;

        Line* current_line = &medit_file_view_file(medit, file_view)->lines.items[cursor->line];
        if (cursor->byte < current_line->count) {
            const char* grapheme = &current_line->items[cursor->byte];
            PixelPos char_pos = {
                .x = on_screen.x - file_view->scrolling.x,
                .y = on_screen.y - file_view->scrolling.y,
            };
            ui_sdl3_draw_text(ui, grapheme, cursor->len, &ui->font_editor, char_pos, glyph_color);
        }
    }
}

void ui_sdl3_scroll_file_view(SDL3Ui* ui, Rect text_area, FileViewGroup* group)
{
    FileView* file_view = medit_get_displayed_file_view_in_group(ui->medit, group);
    Cursor* cursor = &file_view->cursors.items[0];
    const Rect on_screen = ui_sdl3_cursor_rect(ui, text_area, cursor, file_view);

    const int32_t margin_x = size_to_i32(ui->font_editor.default_cursor_width * 3);
    const int32_t margin_y = size_to_i32(ui->font_editor.line_spacing * 3);

    const int32_t right_border = text_area.x + text_area.w - margin_x;
    const int32_t bottom_border = text_area.y + text_area.h - margin_y;
    const int32_t left_border = text_area.x + margin_x;
    const int32_t top_border = text_area.y + margin_y;

    const int32_t cursor_right = on_screen.x + on_screen.w;
    const int32_t cursor_bottom = on_screen.y + on_screen.h;

    // Compute the valid scroll range that keeps the cursor within both margins:
    // smallest offset that prevents the cursor from going past the right/bottom margin
    const int32_t scroll_min_x = SDL_max(cursor_right, right_border) - right_border;
    const int32_t scroll_min_y = SDL_max(cursor_bottom, bottom_border) - bottom_border;
    // largest offset before the cursor goes past the left/top margin
    const int32_t scroll_max_x = SDL_max(on_screen.x, left_border) - left_border;
    const int32_t scroll_max_y = SDL_max(on_screen.y, top_border) - top_border;

    file_view->scrolling.x = SDL_clamp(file_view->scrolling.x, scroll_min_x, scroll_max_x);
    file_view->scrolling.y = SDL_clamp(file_view->scrolling.y, scroll_min_y, scroll_max_y);
}

void ui_sdl3_compute_line_number_gutter_width(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    const int line_count = SDL_max(
        size_to_int(medit_file_view_file(medit, file_view)->lines.count),
        1000);
    if (line_count == ui->line_nr_cached_line_count) {
        return;
    }
    ui->line_nr_cached_line_count = line_count;
    ui->line_nr_max_digits = medit_digits_count(line_count);

    char buffer[INT64_DIGITS_COUNT] = { 0 };
    int written = snprintf(buffer, sizeof(buffer), "%d ", line_count);
    assert(written > 0);

    int line_number_width = 0;
    TTF_MeasureString(
        ui->font_editor.main,
        buffer,
        int_to_size(written),
        0,
        &line_number_width,
        NULL);
    assert(line_number_width >= 0);

    ui->line_nr_padding = line_number_width;
}

static void ui_sdl3_draw_line_number(SDL3Ui* ui, size_t row, Rect gutter, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);
    bool focused = medit_get_focused_file_view_group(medit) == group;

    const Color line_number_color = focused && row == file_view->cursors.items[0].line
        ? medit->config.color_theme.line_number_current
        : medit->config.color_theme.line_number;

    PixelPos pos = {
        .x = gutter.x,
        .y = size_to_i32(row * ui->font_editor.line_spacing) + gutter.y - file_view->scrolling.y,
    };

    char line_number[INT64_DIGITS_COUNT] = { 0 };
    int written = snprintf(
        line_number,
        sizeof(line_number),
        "%*zu",
        ui->line_nr_max_digits,
        row + 1);
    size_t line_number_len = int_to_size(written);

    ui_sdl3_draw_text(ui, line_number, line_number_len, &ui->font_editor, pos, line_number_color);
}

static void ui_sdl3_draw_line(
    SDL3Ui* ui,
    size_t row,
    Line* line,
    Rect content,
    FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* file_view = medit_get_displayed_file_view_in_group(medit, group);

    PixelPos line_pos = {
        .x = content.x - file_view->scrolling.x,
        .y = size_to_i32(row * ui->font_editor.line_spacing) + content.y - file_view->scrolling.y,
    };

    ui_sdl3_draw_text(
        ui,
        line->items,
        line->count,
        &ui->font_editor,
        line_pos,
        medit->config.color_theme.editor_fg);
}

// Draw file lines and line numbers directly to the renderer
// Must be called after the bg draw layer has been flushed
void ui_sdl3_draw_file_view_group_content(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    FileView* displayed_file_view = medit_get_displayed_file_view_in_group(medit, group);
    Lines* lines = &medit_file_view_file(medit, displayed_file_view)->lines;

    const size_t first_rendered_line = (size_t)SDL_max(displayed_file_view->scrolling.y, 0)
        / ui->font_editor.line_spacing;
    const size_t screen_lines = ((size_t)ui->window_size.height / ui->font_editor.line_spacing) + 1;
    const size_t rendered_line_count = SDL_min(lines->count, first_rendered_line + screen_lines);

    Rect area = group->content_area;
    Rect gutter = rect_cut_left(&area, ui->line_nr_padding);
    Rect content = area;

    const SDL_Rect gutter_clip = rect_to_sdl_rect(gutter);
    assert(SDL_SetRenderClipRect(ui->renderer, &gutter_clip));
    for (size_t row = first_rendered_line; row < rendered_line_count; ++row) {
        ui_sdl3_draw_line_number(ui, row, gutter, group);
    }

    const SDL_Rect content_clip = rect_to_sdl_rect(content);
    assert(SDL_SetRenderClipRect(ui->renderer, &content_clip));
    for (size_t row = first_rendered_line; row < rendered_line_count; ++row) {
        ui_sdl3_draw_line(ui, row, &lines->items[row], content, group);
    }

    assert(SDL_SetRenderClipRect(ui->renderer, NULL));
}
