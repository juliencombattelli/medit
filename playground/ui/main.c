#include <core/assert.h>

#include <default_settings.h>

#include "ui2.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
} AppState;

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

// void ui2_sdl3_flush_draw_list(SDL3Ui* ui, Ui2DrawCmdList list)
// {
//     for (size_t i = 0; i < list.count; i++) {
//         const Ui2DrawCmd* cmd = &list.items[i];
//         switch (cmd->kind) {
//             case UI2_DRAW_CMD_RECT_FILLED: {
//                 SDL_FRect r = ui2_bb_to_sdl_frect(cmd->bounding_box);
//                 SDL_SetRenderDrawColor(
//                     ui->renderer,
//                     color_to_RGBA_args(cmd->rect.background_color));
//                 SDL_RenderFillRect(ui->renderer, &r);
//             } break;
//             case UI2_DRAW_CMD_TEXT: {
//                 ui_sdl3_draw_text(
//                     ui,
//                     cmd->text.text,
//                     cmd->text.length,
//                     &ui->font_editor,
//                     (PixelPos) { .x = (int)cmd->bounding_box.pos.x,
//                                  .y = (int)cmd->bounding_box.pos.y },
//                     ui2_color_to_sdl_color(cmd->text.color));
//             } break;
//             case UI2_DRAW_CMD_CLIP_PUSH: {
//                 SDL_Rect r = ui2_bb_to_sdl_rect(cmd->bounding_box);
//                 SDL_SetRenderClipRect(ui->renderer, &r);
//             } break;
//             case UI2_DRAW_CMD_CLIP_POP: {
//                 SDL_SetRenderClipRect(ui->renderer, NULL);
//             } break;
//             default: abort();
//         }
//     }
// }

Ui2DrawCmdList editor_layout(Ui2Context* ctx)
{
    ui2_frame_begin(ctx);

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
    AppState state = { 0 };

    assert(SDL_Init(SDL_INIT_VIDEO));
    assert(TTF_Init());

    state.window = SDL_CreateWindow(
        "UI Playground",
        1280,
        720,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    assert(state.window);

    state.renderer = SDL_CreateRenderer(state.window, NULL);
    assert(state.renderer);

    assert(SDL_SetRenderVSync(state.renderer, 1));

    state.text_engine = TTF_CreateRendererTextEngine(state.renderer);
    assert(state.text_engine);

    assert(SDL_ShowWindow(state.window));
    assert(SDL_StartTextInput(state.window));

    bool running = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_WINDOW_RESIZED: break;
                case SDL_EVENT_KEY_DOWN: break;
                case SDL_EVENT_TEXT_INPUT: break;
                case SDL_EVENT_KEYMAP_CHANGED: break;
                case SDL_EVENT_MOUSE_WHEEL: break;
                default: break;
            }
        }

        SDL_SetRenderDrawColor(state.renderer, 0, 128, 0, 255);
        SDL_RenderClear(state.renderer);

        SDL_RenderPresent(state.renderer);
    }

    SDL_StopTextInput(state.window);

    TTF_DestroyRendererTextEngine(state.text_engine);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

    TTF_Quit();
    SDL_Quit();
}
