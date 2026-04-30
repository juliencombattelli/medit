#include "sdl3.h"

#include "sdl3_internal.h"

#include <core/assert.h>
#include <core/dynarray.h>
#include <core/unicode.h>
#include <core/utils.h>

#include <string.h>

// TODO temporary function placing groups on screen till we have a proper layout engine
static void temp_ui_sdl3_update_file_view_groups_size(SDL3Ui* ui)
{
    ui->layout.sizes = UI_SDL3_DEFAULT_LAYOUT_SIZES;
    ui->layout.elements_shown = LAYOUT_MENU_BAR | LAYOUT_STATUS_BAR | LAYOUT_TAB_BAR
        | LAYOUT_SIDE_PANEL;
    ui_sdl3_recompute_layout(ui);
}

static void temp_ui_sdl3_setup_layout(SDL3Ui* ui)
{
    Meditor* medit = ui->medit;

    // Create an empty file in some file view groups
    for (size_t i = 0; i < 3; ++i) {
        dynarray_append(&medit->file_views, (FileViewGroup) { 0 });
        medit->file_views.focused = medit->file_views.count - 1;
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_load_file(medit, "./src/ui/sdl3/sdl3.c");
    }

    // Insert some text in the focused latest created group
    const char text[] = "😊😊😊😊😊😊ùùùù😊";
    medit_insert_text(medit, text, strlen(text));

    // Update the layout of the groups in a grid fashion
    temp_ui_sdl3_update_file_view_groups_size(ui);
}

static void report_perf_counter(PerfCounter* perf_counter, void* userdata)
{
    MEDIT_UNUSED(userdata);

    SDL_Log(
        "Active frames: %llu | Avg frame time: %.2fms",
        (unsigned long long)perf_counter->frame_count,
        SDL_NS_TO_MS((double)perf_counter->accumulated_ns) / (double)perf_counter->frame_count);
}

void medit_ui_sdl3_run(Meditor* medit)
{
    SDL3Ui ui = { 0 };
    assert(ui_sdl3_create(&ui, medit));

    ui_sdl3_load_editor_font(&ui);

    temp_ui_sdl3_setup_layout(&ui);

    for (size_t i = 0; i < medit->file_views.count; ++i) {
        FileViewGroup* group = &medit->file_views.items[i];
        for (size_t j = 0; j < group->count; ++j) {
            FileView* file_view = &group->items[j];
            Cursor* cursor = &file_view->cursors.items[0];
            Line* line = &medit_file_view_file(medit, file_view)->lines.items[0];

            UcGraphemeIter it = { 0 };
            uc_grapheme_iter_init(&it, (uint8_t*)line->items, line->count, cursor->byte);
            UcSpan out = { 0 };
            uc_grapheme_iter_next(&it, &out);
            cursor->len = out.len;
            printf("group %zu, fileview %zu, cursor->len=%zu\n", i, j, cursor->len);
        }
    }

    ui_sdl3_enable_cursor_blink(&ui);

    perf_counter_start_periodic_report(
        &ui.perf_counter,
        PERF_COUNTER_REPORT_PERIOD_MS,
        report_perf_counter,
        NULL);

    medit->running = true;
    while (medit->running) {
        bool should_render = ui_sdl3_handle_event(&ui);
        if (ui.editor_font_size != medit->config.editor_font_size) {
            ui_sdl3_unload_editor_font(&ui);
            ui_sdl3_load_editor_font(&ui);
            should_render = true;
        }

        if (!should_render) {
            continue;
        }

        ui_sdl3_clear(&ui);
        ui_sdl3_draw_frame_begin(&ui);
        {
            // 1. Queue background layer: panels, tab bars, group backgrounds
            ui_sdl3_draw_panels(&ui);
            ui_panel(&ui.ui_ctx_bg, ui.layout.editor_area, medit->config.color_theme.panel_border);

            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                ui_sdl3_compute_line_number_gutter_width(&ui, group);
                ui_sdl3_draw_file_view_group(&ui, group);
            }

            // 2. Flush background layer so file content is drawn on top
            ui_sdl3_flush_draw_list(&ui, &ui.ui_draw_list_bg);

            // 3. Draw file content directly (lines + line numbers)
            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                Rect text_area = group->content_area;
                rect_cut_left(&text_area, ui.line_nr_padding);
                ui_sdl3_scroll_file_view(&ui, text_area, group);
                ui_sdl3_draw_file_view_group_content(&ui, group);
                // 4. Queue cursor rects into overlay layer
                ui_sdl3_queue_cursor(&ui, text_area, group);
            }

            // 5. Flush overlay layer (cursor rects) on top of file content
            ui_sdl3_flush_draw_list(&ui, &ui.ui_draw_list_overlay);

            // 6. Draw cursor glyphs on top of cursor rects
            for (size_t i = 0; i < medit->file_views.count; ++i) {
                FileViewGroup* group = &medit->file_views.items[i];
                Rect text_area = group->content_area;
                rect_cut_left(&text_area, ui.line_nr_padding);
                ui_sdl3_draw_cursor_glyphs(&ui, text_area, group);
            }
        }
        ui_sdl3_render_frame(&ui);

        perf_counter_frame_end(&ui.perf_counter);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
