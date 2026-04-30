#include "sdl3_internal.h"

#include <core/dynarray.h>
#include <core/safeint.h>
#include <core/utils.h>

void ui_sdl3_recompute_layout(SDL3Ui* ui)
{
    medit_layout_recompute(
        &ui->layout,
        &ui->medit->file_views,
        int_to_size(ui->window_size.width),
        int_to_size(ui->window_size.height));
}

// Enough to hold 2 64-bit integers and some text
#define CURSOR_POS_SEGMENT_LENGTH ((INT64_DIGITS_COUNT * 2) + 32)

static void ui_sdl3_draw_status_bar_text(SDL3Ui* ui)
{
    FileView* file_view = medit_get_focused_file_view(ui->medit);
    Cursor* cursor = &file_view->cursors.items[0];
    size_t col = cursor->grapheme_col + 1;
    size_t line = cursor->line + 1;
    char cursor_pos_segment[CURSOR_POS_SEGMENT_LENGTH] = { 0 };
    int written = snprintf(
        cursor_pos_segment,
        sizeof(cursor_pos_segment),
        "Ln %zu, Col %zu ",
        line,
        col);
    size_t len = int_to_size(written);

    int segment_width = 0;
    TTF_MeasureString(ui->font_editor.main, cursor_pos_segment, len, 0, &segment_width, NULL);

    Panel* status_bar = &ui->layout.status_bar;
    int font_h = TTF_GetFontHeight(ui->font_editor.main);
    int text_x = status_bar->area.x + status_bar->area.w - segment_width;
    int text_y = status_bar->area.y + ((status_bar->area.h - font_h) / 2);

    const char* status_text = ui_sdl3_arena_str(ui, cursor_pos_segment, len);
    if (!status_text) {
        return;
    }

    UiDrawCmd text_cmd = {
        .kind = UI_CMD_TEXT,
        .rect = {
            .x = text_x,
            .y = text_y,
            .w = segment_width,
            .h = status_bar->area.h,
        },
        .color = ui->medit->config.color_theme.editor_fg,
        .text = status_text,
    };
    dynarray_append(&ui->ui_draw_list_bg, text_cmd);
}

void ui_sdl3_draw_panels(SDL3Ui* ui)
{
    const ColorTheme* theme = &ui->medit->config.color_theme;

    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_MENU_BAR)) {
        ui_sdl3_draw_panel(ui, ui->layout.menu_bar, theme->menu_bar_bg);
    }
    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_STATUS_BAR)) {
        ui_sdl3_draw_panel(ui, ui->layout.status_bar, theme->status_bar_bg);
        ui_sdl3_draw_status_bar_text(ui);
    }
    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_SIDE_PANEL)) {
        ui_sdl3_draw_panel(ui, ui->layout.side_panel, theme->sidebar_bg);
    }
    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_BOTTOM_PANEL)) {
        ui_sdl3_draw_panel(ui, ui->layout.bottom_panel, theme->bottom_panel_bg);
    }
}
