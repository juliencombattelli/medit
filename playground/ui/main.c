#include <ui/sdl3/sdl3_internal.h>
#include <ui/sdl3/utils/utils.h>

#include <core/assert.h>

#include <default_settings.h>

#include "ui2.h"

SDL_Rect ui2_bb_to_sdl_rect(Ui2BoundingBox bb)
{
    return (SDL_Rect) {
        .x = bb.pos.x,
        .y = bb.pos.y,
        .w = bb.dim.width,
        .h = bb.dim.height,
    };
}

SDL_FRect ui2_bb_to_sdl_frect(Ui2BoundingBox bb)
{
    return (SDL_FRect) {
        .x = (float)bb.pos.x,
        .y = (float)bb.pos.y,
        .w = (float)bb.dim.width,
        .h = (float)bb.dim.height,
    };
}

Color ui2_color_to_sdl_color(Ui2Color color)
{
    return (Color) {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
    };
}

void ui2_sdl3_flush_draw_list(SDL3Ui* ui, Ui2DrawCmdList list)
{
    for (size_t i = 0; i < list.count; i++) {
        const Ui2DrawCmd* cmd = &list.items[i];
        switch (cmd->kind) {
            case UI2_DRAW_CMD_RECT_FILLED: {
                SDL_FRect r = ui2_bb_to_sdl_frect(cmd->bounding_box);
                SDL_SetRenderDrawColor(
                    ui->renderer,
                    color_to_RGBA_args(cmd->rect.background_color));
                SDL_RenderFillRect(ui->renderer, &r);
            } break;
            case UI2_DRAW_CMD_TEXT: {
                ui_sdl3_draw_text(
                    ui,
                    cmd->text.text,
                    cmd->text.length,
                    &ui->font_editor,
                    (PixelPos) { .x = (int)cmd->bounding_box.pos.x,
                                 .y = (int)cmd->bounding_box.pos.y },
                    ui2_color_to_sdl_color(cmd->text.color));
            } break;
            case UI2_DRAW_CMD_CLIP_PUSH: {
                SDL_Rect r = ui2_bb_to_sdl_rect(cmd->bounding_box);
                SDL_SetRenderClipRect(ui->renderer, &r);
            } break;
            case UI2_DRAW_CMD_CLIP_POP: {
                SDL_SetRenderClipRect(ui->renderer, NULL);
            } break;
            default: abort();
        }
    }
}

Ui2DrawCmdList editor_layout(Ui2Context* ctx)
{
    ui2_frame_begin(&ctx);

    // CLAY(CLAY_ID("window_frame"), {
    //         .layout = {
    //           .layoutDirection = CLAY_TOP_TO_BOTTOM,
    //           .sizing = layout_expand,
    //           .padding = CLAY_PADDING_ALL(16),
    //           .childGap = 16,
    //         },
    //         .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF},
    //     }) {
    //     CLAY(
    //         CLAY_ID("menu_bar"), {
    //             .layout = {
    //                 .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_GROW(0), },
    //             },
    //             .backgroundColor = sidebars_background_color,
    //             .cornerRadius = CLAY_CORNER_RADIUS(8),
    //         })
    //     {
    //     }
    //     CLAY(
    //         CLAY_ID("editor_area"), {
    //             .layout = {
    //                 .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0), },
    //             },
    //             .backgroundColor = editor_background_color,
    //             .cornerRadius = CLAY_CORNER_RADIUS(8),
    //         })
    //     {
    //     }
    //     CLAY(
    //         CLAY_ID("status_bar"), {
    //             .layout = {
    //                 .sizing = { .height = CLAY_SIZING_FIXED(30), .width = CLAY_SIZING_GROW(0), },
    //             },
    //             .backgroundColor = sidebars_background_color,
    //             .cornerRadius = CLAY_CORNER_RADIUS(8),
    //         })
    //     {
    //     }
    // }
    return ui2_frame_end(ctx);
}

int main(void)
{
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;
    medit.config.color_theme = default_color_theme();

    Ui2Context ctx;
    ui2_init(&ctx, (Ui2Arena) { 0 });

    SDL3Ui ui = { 0 };
    assert(ui_sdl3_create(&ui, &medit));

    ui_sdl3_load_editor_font(&ui);

    ui_sdl3_enable_cursor_blink(&ui);

    medit.running = true;
    while (medit.running) {
        bool should_render = ui_sdl3_handle_event(&ui);
        if (!should_render) {
            continue;
        }

        Ui2DrawCmdList draw_cmd_list = editor_layout(&ctx);

        ui_sdl3_clear(&ui);
        ui2_sdl3_flush_draw_list(&ui, draw_cmd_list);
        ui_sdl3_render_frame(&ui);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
