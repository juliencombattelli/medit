#include "sdl3.h"

#include "sdl3_internal.h"
#include "sdl3_clay_renderer.h"

#include "default_config.h"

#include <core/assert.h>
#include <core/dynarray.h>
#include <core/unicode.h>
#include <core/utils.h>

#include <string.h>

static void temp_display_sdl3_setup_layout(SDL3Display* display)
{
    Meditor* medit = display->medit;

#if 0
    // Create an empty file in some file view groups
    for (size_t i = 0; i < 3; ++i) {
        dynarray_append(&medit->file_views, (FileViewGroup) { 0 });
        medit->file_views.focused = medit->file_views.count - 1;
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
        medit_load_file(medit, "./src/display/sdl3/sdl3.c");
    }

    // Insert some text in the focused latest created group
    const char text[] = "😊😊😊😊😊😊ùùùù😊";
    medit_insert_text(medit, text, strlen(text));

    // Update the layout of the groups in a grid fashion
    temp_display_sdl3_update_file_view_groups_size(ui);
#else
    dynarray_append(&medit->file_views, (FileViewGroup) { 0 });
    medit_new_empty_file(medit, &dynarray_last(&medit->file_views));
#endif
}

static Clay_RenderCommandArray medit_layout(Ui* ui)
{
    Clay_BeginLayout();

    CLAY(CLAY_ID("window_frame"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {
                .width = CLAY_SIZING_GROW(0),
                .height = CLAY_SIZING_GROW(0),
            },
        },
        .backgroundColor = { 0x7F, 0x00, 0x00, 0xFF},
    }) {
        medit_ui_layout_titlebar(ui);

        CLAY(CLAY_ID("window_frame_inner"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = ui->theme.layout_settings.panels_gap,
            },
            .backgroundColor = { 0x7F, 0x00, 0x00, 0xFF},
        }) {
            // menu_bar_layout(ui);
            // middle_area_layout(ui);
            // status_bar_layout(ui);
        }
    }

    // TODO handle dragged element
    // Clay_ElementId dragged_menu_bar_element_id = Clay_GetElementId(CLAY_STRING("dragged_menu_bar_element"));

    // if (dragged_menu_bar_element != NO_DRAGGED_MENU_BAR_ELEMENT
    //     && ui->drag_state.active_id == dragged_menu_bar_element_id.id)
    // {
    //     Clay_ElementData bar_elem = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("menu_bar")));
    //     Clay_PointerData ptr = Clay_GetPointerState();
    //     bool inside = bar_elem.found && point_inside_rect(ptr.position, bar_elem.boundingBox);
    //     if (inside) {
    //         dragged_menu_bar_element_drop_indicator_layout(ui);
    //     }
    //     dragged_menu_bar_element_layout(ui);
    // }

    return Clay_EndLayout(0);
}

static void report_perf_counter(PerfCounter* perf_counter, void* userdata)
{
    MEDIT_UNUSED(userdata);

    SDL_Log(
        "Active frames: %llu | Avg frame time: %.2fms",
        (unsigned long long)perf_counter->frame_count,
        SDL_NS_TO_MS((double)perf_counter->accumulated_ns) / (double)perf_counter->frame_count);
}

static void handle_clay_errors(Clay_ErrorData error_data)
{
    printf("%s\n", error_data.errorText.chars);
}

static void display_sdl3_setup_clay(SDL3Display* display, Ui* ui)
{
    int width = 0, height = 0;
    SDL_GetWindowSize(display->window, &width, &height);
    uint32_t mem_size = Clay_MinMemorySize();
    Clay_Context* context = Clay_Initialize(
        Clay_CreateArenaWithCapacityAndMemory(mem_size, malloc(mem_size)),
        (Clay_Dimensions) { .width = (float)width, .height = (float)height },
        (Clay_ErrorHandler) { .errorHandlerFunction = handle_clay_errors });
    assert(context != NULL);
}

static inline uint32_t sdl_get_mouse_state(SDL_Window* window, MouseState* mouse_state)
{
    // Use SDL_GetGlobalMouseState as SDL_GetMouseState may not get an up-to-date state when mouse is over custom window
    // decorations on Windows

    float gx = 0, gy = 0;
    uint32_t mouse_buttons = SDL_GetGlobalMouseState(&gx, &gy);

    int wx = 0, wy = 0;
    SDL_GetWindowPosition(window, &wx, &wy);

    mouse_state->pos.x = gx - (float)wx;
    mouse_state->pos.y = gy - (float)wy;

    // printf("mouse_buttons=%u, x=%f, y=%f\n", mouse_buttons, mouse_state->pos.x, mouse_state->pos.y);

    return mouse_buttons;
}

void medit_ui_titlebar_init(Ui* ui, SDL_Window* window);

MeditAppResult medit_display_sdl3_run(void* old_display_state)
{
    bool reload_requested = old_display_state != NULL;
    SDL3Display* display = (SDL3Display*)old_display_state;
    Meditor* medit = NULL;

    Ui ui = {
        .theme = {
            .color_scheme = {
                .scrollbar_thumb_inactive = { 100 + 100, 100, 100, 150 },
                .scrollbar_thumb_hovered  = { 120 + 100, 120, 120, 150 },
                .scrollbar_thumb_active   = { 140 + 100, 140, 140, 150 },
                .titlebar_ctrl_button_minimize = { 0xFF, 0xFF, 0x00, 0xFF},
                .titlebar_ctrl_button_maximize = { 0xFF, 0x00, 0xFF, 0xFF},
                .titlebar_ctrl_button_close    = { 0x00, 0xFF, 0xFF, 0xFF},
            },
            .layout_settings = {
                .scrollbar_corner_radius = 6,
                .scrollbar_size = 12,
                .dragged_tab_transparency = 0x9F,
                .panels_gap = 8,
                .panels_corner_radius = {8, 8, 8, 8},
                .titlebar_height = 32,
                .titlebar_button_width = 46,
                .window_resize_border = 4,
            },
        },
    };

    if (!reload_requested) {
        display = (SDL3Display*)calloc(1, sizeof(SDL3Display));
        display->medit = (Meditor*)calloc(1, sizeof(Meditor));
        medit = display->medit;
        medit->config = medit_default_config();

        if (!display_sdl3_create(display, medit)) {
            return (MeditAppResult) { .return_code = MEDIT_STATUS_FAILED_TO_CREATE_GUI };
        }

        display_sdl3_ttf_setup(display);

        display_sdl3_setup_clay(display, &ui);

        medit_ui_titlebar_init(&ui, display->window);

        temp_display_sdl3_setup_layout(display);

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

    } else {
        medit = display->medit;
        reload_requested = false;
        // The keybinding table lives in the persistent Meditor state, but its
        // action callbacks are function pointers into this shared library. After
        // a hot reload the library has been unloaded and reloaded (potentially at
        // different addresses), so the previously stored pointers are stale and
        // would jump into invalid/old code on the next keypress. Re-bind them so
        // they resolve to this freshly loaded library.
        medit_load_default_keybind_full(medit, &DISPLAY_SDL3_ACTIONS, display);
        display_sdl3_ttf_setup(display);
        printf("Application hot-reloaded\n");
    }

    display_sdl3_enable_cursor_blink(display);

    perf_counter_start_periodic_report(
        &display->perf_counter,
        PERF_COUNTER_REPORT_PERIOD_MS,
        report_perf_counter,
        NULL);

    medit->running = true;
    while (medit->running) {
        EventReaction reaction = display_sdl3_handle_event(display);
        if (display->editor_font_size != medit->config.editor_font_size) {
            display_sdl3_unload_font(display, FONT_ID_EDITOR);
            display_sdl3_load_font(display, FONT_ID_EDITOR);
            reaction |= REQUEST_RENDER;
        }

        if (reaction & REQUEST_HOT_RELOADING) {
            perf_counter_stop_periodic_report(&display->perf_counter);
            display_sdl3_disable_cursor_blink(display);
            display_sdl3_ttf_teardown(display);
            return (MeditAppResult) { .should_reload = true, .app_state = display };
        }

        if (!(reaction & REQUEST_RENDER)) {
            continue;
        }

        Clay_SetLayoutDimensions((Clay_Dimensions) {
            (float)display->window_size.width,
            (float)display->window_size.height,
        });

        mouse_state_update(&ui.mouse_state, sdl_get_mouse_state(display->window, &ui.mouse_state));

        Clay_SetPointerState(ui.mouse_state.pos,
            ui.mouse_state.buttons[MOUSE_BUTTON_LEFT].state < MOUSE_BUTTON_PRESSED);

        medit_ui_update_scroll_containers(&ui);

        medit_ui_update_titlebar(&ui, display->window_size.width);

        if (ui.mouse_state.buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_RELEASED_THIS_FRAME) {
            if (ui.titlebar_state.hovered_button == TITLEBAR_BTN_MIN) {
                SDL_MinimizeWindow(display->window);
            }
            if (ui.titlebar_state.hovered_button == TITLEBAR_BTN_MAX) {
                if ((SDL_GetWindowFlags(display->window) & SDL_WINDOW_MAXIMIZED) != 0) {
                    SDL_RestoreWindow(display->window);
                } else {
                    SDL_MaximizeWindow(display->window);
                }
            }
            if (ui.titlebar_state.hovered_button == TITLEBAR_BTN_CLOSE) {
                medit->running = false;
            }
        }

        // {
        //     for (size_t i = 0; i < medit->file_views.count; ++i) {
        //         FileViewGroup* group = &medit->file_views.items[i];
        //         display_sdl3_compute_line_number_gutter_width(display, group);
        //         display_sdl3_draw_file_view_group_content(display, group);
        //     }

        //     // TODO iterate on all file views
        //     {
        //         FileViewGroup* group = &medit->file_views.items[0];
        //         Rect text_area = group->content_area;
        //         display_sdl3_scroll_file_view(display, text_area, group);
        //         display_sdl3_draw_file_view_group_content(display, group);
        //         display_sdl3_queue_cursor(display, text_area, group);
        //     }

        //     // Draw cursor glyphs on top of cursor rects
        //     // TODO iterate on all file views
        //     {
        //         FileViewGroup* group = &medit->file_views.items[0];
        //         Rect text_area = group->content_area;
        //         display_sdl3_draw_cursor_glyphs(display, text_area, group);
        //     }
        // }

        Clay_RenderCommandArray render_commands = medit_layout(&ui);

        display_sdl3_clear(display);
        SDL_Clay_RenderClayCommands(display, &render_commands);
        display_sdl3_render_frame(display);

        perf_counter_frame_end(&display->perf_counter);
    }

    perf_counter_stop_periodic_report(&display->perf_counter);
    display_sdl3_disable_cursor_blink(display);

    display_sdl3_ttf_teardown(display);
    display_sdl3_destroy(display);

    medit_close_all_files(medit);

    free(medit);
    free(display);

    return (MeditAppResult) { .return_code = MEDIT_STATUS_SUCCESS };
}
