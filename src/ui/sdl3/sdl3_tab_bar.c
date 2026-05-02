#include "sdl3_internal.h"

#include <core/dynarray.h>
#include <core/safeint.h>
#include <core/string_view.h>
#include <core/utils.h>

typedef struct {
#define FILE_VIEW_TAB_TEXT_CONTENT_LEN 1024
    // TODO when scrollable tab bars will be supported, there will be no real limit to the tab
    // content length
    char content[FILE_VIEW_TAB_TEXT_CONTENT_LEN];
    size_t length;
    int32_t width;
} FileViewTabText;

static void ui_sdl3_format_file_view_tab_text(
    SDL3Ui* ui,
    FileView* file_view,
    FileViewTabText* tab_text)
{
    // TODO draw a button instead of just an indicator
    static const char dirty_indicator[] = " ●";

    Meditor* medit = ui->medit;
    File* file = medit_file_view_file(medit, file_view);

    StringView filename = sv_path_basename(sv_from_cstr(file->name));

    const char* dirty_str = file->dirty ? dirty_indicator : "";
    int written = snprintf(
        tab_text->content,
        sizeof(tab_text->content),
        " " SV_Fmt "%s ",
        SV_Arg(filename),
        dirty_str);
    tab_text->length = int_to_size(written);

    int w = 0;
    TTF_MeasureString(ui->font_editor.main, tab_text->content, tab_text->length, 0, &w, NULL);
    tab_text->width = w;
}

static void ui_sdl3_draw_tab_bar_tabs(
    SDL3Ui* ui,
    FileViewGroup* group,
    Rect tabs_viewport,
    Rect scroll_area)
{
    const LayoutSizes s = ui->layout.sizes;

    int32_t cursor_x = scroll_area.x;
    const int font_h = TTF_GetFontHeight(ui->font_editor.main);

    for (size_t i = 0; i < group->count; ++i) {
        FileView* file_view = &group->items[i];
        FileViewTabText tab_text = { 0 };
        ui_sdl3_format_file_view_tab_text(ui, file_view, &tab_text);
        const int32_t tab_w = SDL_max(tab_text.width, 128);
        const int32_t tab_right = cursor_x + tab_w;

        // Skip tabs fully off-screen to the left
        if (tab_right <= tabs_viewport.x) {
            cursor_x = tab_right + s.separator_size;
            continue;
        }
        // Stop when fully off-screen to the right
        if (cursor_x >= tabs_viewport.x + tabs_viewport.w) {
            break;
        }

        // Clip to 0 for the Rect when negative; the clip rect handles the visible boundary
        Rect tab_area = {
            .x = SDL_max(cursor_x, 0),
            .y = tabs_viewport.y,
            .w = tab_w,
            .h = tabs_viewport.h,
        };
        const Color tab_color = i == group->displayed ? ui->medit->config.color_theme.editor_bg
                                                      : ui->medit->config.color_theme.tab_bar_bg;
        ui_panel(&ui->ui_ctx_bg, tab_area, tab_color);

        if (s.separator_size > 0) {
            Rect sep_area = {
                .x = SDL_max(tab_right, 0),
                .y = tabs_viewport.y,
                .w = s.separator_size,
                .h = tabs_viewport.h,
            };
            ui_panel(&ui->ui_ctx_bg, sep_area, ui->medit->config.color_theme.panel_border);
        }

        const int tab_text_y = tabs_viewport.y + ((tabs_viewport.h - font_h) / 2);
        const char* tab_label = ui_sdl3_arena_str(ui, tab_text.content, tab_text.length);
        if (tab_label) {
            UiDrawCmd cmd = {
                .kind = UI_CMD_TEXT,
                .rect = {
                    .x = cursor_x,
                    .y = tab_text_y,
                    .w = tab_area.w,
                    .h = tab_area.h,
                },
                .color = ui->medit->config.color_theme.editor_fg,
                .text = tab_label,
            };
            dynarray_append(&ui->ui_draw_list_bg, cmd);
        }

        cursor_x = tab_right + s.separator_size;
    }
}

static Color get_thumb_color(
    SDL3Ui* ui,
    FileViewGroup* group,
    bool tab_bar_hovered,
    bool scrollbar_hovered)
{
    const ColorTheme* color_theme = &ui->medit->config.color_theme;

    const bool no_scrollbar_dragged = !ui->ui_scrollbar_dragged;
    const bool current_scrollbar_dragged = ui->ui_scrollbar_dragged
        && group->tab_bar_scroll.drag_active_h;

    Color thumb_color = color_theme->scrollbar_thumb;
    if (tab_bar_hovered && no_scrollbar_dragged) {
        thumb_color = color_theme->scrollbar_thumb_scroll_area_hovered;
    }
    if (scrollbar_hovered || current_scrollbar_dragged) {
        thumb_color = color_theme->scrollbar_thumb_hovered;
    }
    return thumb_color;
}

static void ui_sdl3_draw_file_view_group_tab_bar(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;
    const LayoutSizes s = ui->layout.sizes;
    Rect tab_bar_area = group->area;
    Panel tab_bar = panel_cut_top(&tab_bar_area, s.tab_bar_height, s.separator_size);
    ui_sdl3_draw_panel(ui, tab_bar, medit->config.color_theme.tab_bar_bg);

    int32_t total_tabs_w = 0;
    for (size_t i = 0; i < group->count; ++i) {
        FileViewTabText tab_text = { 0 };
        ui_sdl3_format_file_view_tab_text(ui, &group->items[i], &tab_text);
        total_tabs_w += SDL_max(tab_text.width, 128) + s.separator_size;
    }

    const bool tab_bar_overflow = total_tabs_w > tab_bar.area.w;
    const bool tab_bar_hovered = rect_contains(
        tab_bar.area,
        ui->ui_ctx_bg.input.x,
        ui->ui_ctx_bg.input.y);

    enum {
        TAB_SCROLLBAR_H = 6
    };
    const Rect tabs_viewport = tab_bar.area;
    const Rect scrollbar_track = {
        .x = tab_bar.area.x,
        .y = tab_bar.area.y + tab_bar.area.h - TAB_SCROLLBAR_H,
        .w = tab_bar.area.w,
        .h = TAB_SCROLLBAR_H,
    };

    const float max_offset_x = tab_bar_overflow ? (float)(total_tabs_w - tabs_viewport.w) : 0.0f;
    group->tab_bar_scroll.content_w = (float)total_tabs_w;
    group->tab_bar_scroll.content_h = (float)tabs_viewport.h;
    float delta = 0;
    if (tab_bar_hovered && ui->ui_ctx_bg.input.scroll_valid) {
        const float speed = ui->ui_ctx_bg.scroll_speed;
        delta = (ui->ui_ctx_bg.input.scroll_x * speed) + (ui->ui_ctx_bg.input.scroll_y * speed);
    }
    group->tab_bar_scroll.offset_x = medit_clampf(
        group->tab_bar_scroll.offset_x - delta,
        0.0f,
        max_offset_x);

    const bool no_scrollbar_dragged = !ui->ui_scrollbar_dragged;
    const bool current_scrollbar_dragged = ui->ui_scrollbar_dragged
        && group->tab_bar_scroll.drag_active_h;

    UiWidgetState scrollbar_state = { 0 };
    if (tab_bar_overflow && (no_scrollbar_dragged || current_scrollbar_dragged)) {
        scrollbar_state = ui_scrollbar_h_update(
            &ui->ui_ctx_bg,
            scrollbar_track,
            &group->tab_bar_scroll);
        ui->ui_scrollbar_dragged = group->tab_bar_scroll.drag_active_h;
    }

    Rect scroll_area = ui_scroll_begin(&ui->ui_ctx_bg, tabs_viewport, &group->tab_bar_scroll);
    ui_sdl3_draw_tab_bar_tabs(ui, group, tabs_viewport, scroll_area);
    ui_scroll_end(&ui->ui_ctx_bg, tabs_viewport, &group->tab_bar_scroll, false);

    if (tab_bar_overflow) {
        const Color track_transparent = { 0, 0, 0, 0 };
        const bool scrollbar_hovered = scrollbar_state & UI_STATE_HOVERED;
        Color thumb_color = get_thumb_color(ui, group, tab_bar_hovered, scrollbar_hovered);
        ui_scrollbar_h_draw(
            &ui->ui_ctx_bg,
            scrollbar_track,
            &group->tab_bar_scroll,
            track_transparent,
            thumb_color);
    }
}

// Queue background and tab bar draw commands for this group into the bg draw layer
// Does NOT draw file content lines; call ui_sdl3_draw_file_view_group_content for that
void ui_sdl3_draw_file_view_group(SDL3Ui* ui, FileViewGroup* group)
{
    Meditor* medit = ui->medit;

    ui_panel(&ui->ui_ctx_bg, group->area, medit->config.color_theme.editor_bg);

    if (medit_layout_is_element_shown(&ui->layout, LAYOUT_TAB_BAR)) {
        ui_sdl3_draw_file_view_group_tab_bar(ui, group);
    }
}
