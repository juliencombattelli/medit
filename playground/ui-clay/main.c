#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <ui/sdl3/sdl3_internal.h>
#include <ui/sdl3/utils/utils.h>

#include <core/assert.h>

#include <default_settings.h>

static int NUM_CIRCLE_SEGMENTS = 16;

static void SDL_Clay_RenderFillRoundedRect(
    SDL3Ui* ui,
    const SDL_FRect rect,
    const float cornerRadius,
    const Clay_Color _color)
{
    const SDL_FColor color = { _color.r / 255, _color.g / 255, _color.b / 255, _color.a / 255 };

    int indexCount = 0, vertexCount = 0;

    const float minRadius = SDL_min(rect.w, rect.h) / 2.0f;
    const float clampedRadius = SDL_min(cornerRadius, minRadius);

    const int numCircleSegments = SDL_max(NUM_CIRCLE_SEGMENTS, (int)clampedRadius * 0.5f);

    int totalVertices = 4 + (4 * (numCircleSegments * 2)) + 2 * 4;
    int totalIndices = 6 + (4 * (numCircleSegments * 3)) + 6 * 4;

    SDL_Vertex vertices[totalVertices];
    int indices[totalIndices];

    // define center rectangle
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + clampedRadius, rect.y + clampedRadius },
        color,
        { 0, 0 },
    }; // 0 center TL
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + rect.w - clampedRadius, rect.y + clampedRadius },
        color,
        { 1, 0 },
    }; // 1 center TR
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + rect.w - clampedRadius, rect.y + rect.h - clampedRadius },
        color,
        { 1, 1 },
    }; // 2 center BR
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + clampedRadius, rect.y + rect.h - clampedRadius },
        color,
        { 0, 1 },
    }; // 3 center BL

    indices[indexCount++] = 0;
    indices[indexCount++] = 1;
    indices[indexCount++] = 3;
    indices[indexCount++] = 1;
    indices[indexCount++] = 2;
    indices[indexCount++] = 3;

    // define rounded corners as triangle fans
    const float step = (SDL_PI_F / 2) / numCircleSegments;
    for (int i = 0; i < numCircleSegments; i++) {
        const float angle1 = (float)i * step;
        const float angle2 = ((float)i + 1.0f) * step;

        for (int j = 0; j < 4; j++) { // Iterate over four corners
            float cx, cy, signX, signY;

            switch (j) {
                case 0:
                    cx = rect.x + clampedRadius;
                    cy = rect.y + clampedRadius;
                    signX = -1;
                    signY = -1;
                    break; // Top-left
                case 1:
                    cx = rect.x + rect.w - clampedRadius;
                    cy = rect.y + clampedRadius;
                    signX = 1;
                    signY = -1;
                    break; // Top-right
                case 2:
                    cx = rect.x + rect.w - clampedRadius;
                    cy = rect.y + rect.h - clampedRadius;
                    signX = 1;
                    signY = 1;
                    break; // Bottom-right
                case 3:
                    cx = rect.x + clampedRadius;
                    cy = rect.y + rect.h - clampedRadius;
                    signX = -1;
                    signY = 1;
                    break; // Bottom-left
                default: return;
            }

            vertices[vertexCount++] = (SDL_Vertex) {
                {
                    cx + SDL_cosf(angle1) * clampedRadius * signX,
                    cy + SDL_sinf(angle1) * clampedRadius * signY,
                },
                color,
                { 0, 0 },
            };
            vertices[vertexCount++] = (SDL_Vertex) {
                {
                    cx + SDL_cosf(angle2) * clampedRadius * signX,
                    cy + SDL_sinf(angle2) * clampedRadius * signY,
                },
                color,
                { 0, 0 },
            };

            indices[indexCount++] = j; // Connect to corresponding central rectangle vertex
            indices[indexCount++] = vertexCount - 2;
            indices[indexCount++] = vertexCount - 1;
        }
    }

    // Define edge rectangles
    //  Top edge
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + clampedRadius, rect.y },
        color,
        { 0, 0 },
    }; // TL
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + rect.w - clampedRadius, rect.y },
        color,
        { 1, 0 },
    }; // TR

    indices[indexCount++] = 0;
    indices[indexCount++] = vertexCount - 2; // TL
    indices[indexCount++] = vertexCount - 1; // TR
    indices[indexCount++] = 1;
    indices[indexCount++] = 0;
    indices[indexCount++] = vertexCount - 1; // TR
    // Right edge
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + rect.w, rect.y + clampedRadius },
        color,
        { 1, 0 },
    }; // RT
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + rect.w, rect.y + rect.h - clampedRadius },
        color,
        { 1, 1 },
    }; // RB

    indices[indexCount++] = 1;
    indices[indexCount++] = vertexCount - 2; // RT
    indices[indexCount++] = vertexCount - 1; // RB
    indices[indexCount++] = 2;
    indices[indexCount++] = 1;
    indices[indexCount++] = vertexCount - 1; // RB
    // Bottom edge
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + rect.w - clampedRadius, rect.y + rect.h },
        color,
        { 1, 1 },
    }; // BR
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x + clampedRadius, rect.y + rect.h },
        color,
        { 0, 1 },
    }; // BL

    indices[indexCount++] = 2;
    indices[indexCount++] = vertexCount - 2; // BR
    indices[indexCount++] = vertexCount - 1; // BL
    indices[indexCount++] = 3;
    indices[indexCount++] = 2;
    indices[indexCount++] = vertexCount - 1; // BL
    // Left edge
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x, rect.y + rect.h - clampedRadius },
        color,
        { 0, 1 },
    }; // LB
    vertices[vertexCount++] = (SDL_Vertex) {
        { rect.x, rect.y + clampedRadius },
        color,
        { 0, 0 },
    }; // LT

    indices[indexCount++] = 3;
    indices[indexCount++] = vertexCount - 2; // LB
    indices[indexCount++] = vertexCount - 1; // LT
    indices[indexCount++] = 0;
    indices[indexCount++] = 3;
    indices[indexCount++] = vertexCount - 1; // LT

    // Render everything
    SDL_RenderGeometry(ui->renderer, NULL, vertices, vertexCount, indices, indexCount);
}

void ui_sdl3_draw(SDL3Ui* ui, Clay_RenderCommandArray draw_commands)
{
    for (int32_t i = 0; i < draw_commands.length; i++) {
        Clay_RenderCommand* rcmd = Clay_RenderCommandArray_Get(&draw_commands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        const SDL_FRect rect = { (int)bounding_box.x,
                                 (int)bounding_box.y,
                                 (int)bounding_box.width,
                                 (int)bounding_box.height };

        switch (rcmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* config = &rcmd->renderData.rectangle;
                SDL_SetRenderDrawBlendMode(ui->renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(
                    ui->renderer,
                    color_to_RGBA_args(rcmd->renderData.rectangle.backgroundColor));
                if (config->cornerRadius.topLeft > 0) {
                    SDL_Clay_RenderFillRoundedRect(
                        ui,
                        rect,
                        config->cornerRadius.topLeft,
                        config->backgroundColor);
                } else {
                    SDL_RenderFillRect(ui->renderer, &rect);
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                Clay_BoundingBox boundingBox = rcmd->boundingBox;
                SDL_Rect currentClippingRectangle = {
                    .x = boundingBox.x,
                    .y = boundingBox.y,
                    .w = boundingBox.width,
                    .h = boundingBox.height,
                };
                SDL_SetRenderClipRect(ui->renderer, &currentClippingRectangle);
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                SDL_SetRenderClipRect(ui->renderer, NULL);
            } break;
        }
    }
}

Clay_RenderCommandArray editor_layout(void)
{
    Clay_BeginLayout();

    Clay_Sizing layout_expand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0),
    };
    Clay_Color sidebars_background_color = { 0x18, 0x18, 0x18, 0xFF };
    Clay_Color editor_background_color = { 0x1F, 0x1F, 0x5F, 0xFF };

    CLAY(CLAY_ID("window_frame"), {
            .layout = {
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
              .sizing = layout_expand,
              .padding = CLAY_PADDING_ALL(16),
              .childGap = 16,
            },
            .backgroundColor = { 0xFF, 0xFF, 0xFF, 0xFF},
        }) {
        CLAY(
            CLAY_ID("menu_bar"), {
                .layout = {
                    .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_GROW(0), },
                },
                .backgroundColor = sidebars_background_color,
                .cornerRadius = CLAY_CORNER_RADIUS(8),
            })
        {
        }
        CLAY(
            CLAY_ID("editor_area"), {
                .layout = {
                    .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0), },
                },
                .backgroundColor = editor_background_color,
                .cornerRadius = CLAY_CORNER_RADIUS(8),
            })
        {
        }
        CLAY(
            CLAY_ID("status_bar"), {
                .layout = {
                    .sizing = { .height = CLAY_SIZING_FIXED(30), .width = CLAY_SIZING_GROW(0), },
                },
                .backgroundColor = sidebars_background_color,
                .cornerRadius = CLAY_CORNER_RADIUS(8),
            })
        {
        }
    }
    return Clay_EndLayout(0);
}

int main(void)
{
    Meditor medit = { 0 };

    medit.config.editor_font_size = FONT_SIZE_DEFAULT;
    medit.config.editor_font_path = FONT_PATH_DEFAULT;
    medit.config.color_theme = default_color_theme();

    SDL3Ui ui = { 0 };
    assert(ui_sdl3_create(&ui, &medit));

    ui_sdl3_load_editor_font(&ui);

    ui_sdl3_enable_cursor_blink(&ui);

    uint32_t mem_size = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(mem_size, malloc(mem_size));

    Clay_Initialize(
        arena,
        (Clay_Dimensions) {
            .width = (float)ui.window_size.width,
            .height = (float)ui.window_size.height,
        },
        (Clay_ErrorHandler) { 0 });

    medit.running = true;
    while (medit.running) {
        bool should_render = ui_sdl3_handle_event(&ui);
        if (!should_render) {
            continue;
        }

        Clay_SetLayoutDimensions((Clay_Dimensions) {
            .width = (float)ui.window_size.width,
            .height = (float)ui.window_size.height,
        });

        float mouse_x = 0;
        float mouse_y = 0;
        Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        Clay_SetPointerState(
            (Clay_Vector2) {
                .x = mouse_x,
                .y = mouse_y,
            },
            buttons & SDL_BUTTON_LMASK);

        Clay_RenderCommandArray draw_commands = editor_layout();

        ui_sdl3_clear(&ui);
        ui_sdl3_draw(&ui, draw_commands);
        ui_sdl3_render_frame(&ui);
    }

    ui_sdl3_unload_editor_font(&ui);
    ui_sdl3_destroy(&ui);
}
