#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <core/assert.h>

#include <default_settings.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
} AppState;

static int NUM_CIRCLE_SEGMENTS = 16;

static void SDL_Clay_RenderFillRoundedRect(
    AppState* app_state,
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
    SDL_RenderGeometry(app_state->renderer, NULL, vertices, vertexCount, indices, indexCount);
}

void SDL_Clay_RenderClayCommands(AppState* app_state, Clay_RenderCommandArray draw_commands)
{
    for (int32_t i = 0; i < draw_commands.length; i++) {
        Clay_RenderCommand* rcmd = Clay_RenderCommandArray_Get(&draw_commands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        const SDL_FRect rect = {
            (float)bounding_box.x,
            (float)bounding_box.y,
            (float)bounding_box.width,
            (float)bounding_box.height,
        };

        switch (rcmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* config = &rcmd->renderData.rectangle;
                SDL_SetRenderDrawBlendMode(app_state->renderer, SDL_BLENDMODE_BLEND);
                Clay_Color bg = rcmd->renderData.rectangle.backgroundColor;
                SDL_SetRenderDrawColor(
                    app_state->renderer,
                    (Uint8)bg.r,
                    (Uint8)bg.g,
                    (Uint8)bg.b,
                    (Uint8)bg.a);
                if (config->cornerRadius.topLeft > 0) {
                    SDL_Clay_RenderFillRoundedRect(
                        app_state,
                        rect,
                        config->cornerRadius.topLeft,
                        config->backgroundColor);
                } else {
                    SDL_RenderFillRect(app_state->renderer, &rect);
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                Clay_BoundingBox boundingBox = rcmd->boundingBox;
                SDL_Rect currentClippingRectangle = {
                    .x = (int)boundingBox.x,
                    .y = (int)boundingBox.y,
                    .w = (int)boundingBox.width,
                    .h = (int)boundingBox.height,
                };
                SDL_SetRenderClipRect(app_state->renderer, &currentClippingRectangle);
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                SDL_SetRenderClipRect(app_state->renderer, NULL);
            } break;
        }
    }
}

static const Clay_Color background_color = { 0x18, 0x18, 0x18, 0xFF };
static const Clay_Color sidebars_background_color = { 0x18, 0x7f, 0x18, 0xFF };
static const Clay_Color editor_background_color = { 0x1F, 0x1F, 0x5F, 0xFF };

void menu_bar_layout(void)
{
    CLAY(CLAY_ID("menu_bar"), {
            .layout = {
                .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_GROW(0), },
            },
            .backgroundColor = sidebars_background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
            .clip = { .horizontal = true, .childOffset = Clay_GetScrollOffset() },
        })
    {
        for (size_t i = 0; i < 10; i++) {
            Clay_Color menu_color = editor_background_color;
            menu_color.r += 0xF * i;
            CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_FIXED(200), },
                    },
                    .backgroundColor = menu_color,
                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                })
            {
            }
        }
    }
}

void left_panel_layout(void)
{
    CLAY(CLAY_ID("left_panel"), {
            .layout = {
                .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_FIXED(100), },
            },
            .backgroundColor = sidebars_background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        })
    {
    }
}

void editor_area_layout(void)
{
    CLAY(CLAY_ID("editor_area"), {
            .layout = {
                .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0), },
            },
            .backgroundColor = editor_background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        })
    {
    }
}

void right_panel_layout(void)
{
    CLAY(CLAY_ID("right_panel"), {
            .layout = {
                .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_FIXED(100), },
            },
            .backgroundColor = sidebars_background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        })
    {
    }
}

void middle_area_layout(void)
{
    CLAY(CLAY_ID("middle_area"), {
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0), },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
            },
            .backgroundColor = background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        })
    {
        left_panel_layout();
        editor_area_layout();
        right_panel_layout();
    }
}

void status_bar_layout(void)
{
    CLAY(CLAY_ID("status_bar"), {
            .layout = {
                .sizing = { .height = CLAY_SIZING_FIXED(30), .width = CLAY_SIZING_GROW(0), },
            },
            .backgroundColor = sidebars_background_color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        })
    {
    }
}

Clay_RenderCommandArray editor_layout(void)
{
    Clay_BeginLayout();

    CLAY(CLAY_ID("window_frame"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = 16,
            },
            .backgroundColor = { 0x7F, 0x00, 0x00, 0xFF},
        })
    {
        menu_bar_layout();
        middle_area_layout();
        status_bar_layout();
    }
    return Clay_EndLayout(0);
}

void HandleClayErrors(Clay_ErrorData errorData)
{
    printf("%s", errorData.errorText.chars);
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

    uint32_t mem_size = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(mem_size, malloc(mem_size));
    int width, height;
    SDL_GetWindowSize(state.window, &width, &height);
    Clay_Initialize(
        arena,
        (Clay_Dimensions) { .width = (float)width, .height = (float)height },
        (Clay_ErrorHandler) { .errorHandlerFunction = HandleClayErrors });

    bool running = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_WINDOW_RESIZED:
                    Clay_SetLayoutDimensions((Clay_Dimensions) {
                        (float)event.window.data1,
                        (float)event.window.data2,
                    });
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    printf("mouse wheel: x=%f, y=%f\n", event.wheel.x, event.wheel.y);
                    if ((int)event.wheel.x == 0) {
                        // vertical scrolling
                        Clay_UpdateScrollContainers(
                            true,
                            (Clay_Vector2) { event.wheel.y, event.wheel.y },
                            0.01f);
                    } else if ((int)event.wheel.y == 0) {
                        // horizontal scrolling
                        Clay_UpdateScrollContainers(
                            true,
                            (Clay_Vector2) { event.wheel.x, event.wheel.x },
                            0.01f);
                    }

                    break;
                case SDL_EVENT_KEY_DOWN: break;
                case SDL_EVENT_TEXT_INPUT: break;
                case SDL_EVENT_KEYMAP_CHANGED: break;
                default: break;
            }
        }

        float mouse_x, mouse_y;
        Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        Clay_SetPointerState(
            (Clay_Vector2) { .x = mouse_x, .y = mouse_y },
            buttons & SDL_BUTTON_LMASK);

        Clay_RenderCommandArray render_commands = editor_layout();

        SDL_SetRenderDrawColor(state.renderer, 0, 128, 0, 255);
        SDL_RenderClear(state.renderer);

        SDL_Clay_RenderClayCommands(&state, render_commands);

        SDL_RenderPresent(state.renderer);
    }

    SDL_StopTextInput(state.window);

    TTF_DestroyRendererTextEngine(state.text_engine);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

    TTF_Quit();
    SDL_Quit();
}
