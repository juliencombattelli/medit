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

static size_t dragged_menu_bar_element = (size_t)-1;

typedef struct {
    Clay_Color color;
    float width;
} MenuBarElementData;

static MenuBarElementData menu_bar_elements[] = {
    { { 0x1F, 0x1F, 0x5F, 0xFF }, 200 }, //
    { { 0x2E, 0x2E, 0x6E, 0xFF }, 250 }, //
    { { 0x3D, 0x3D, 0x7D, 0xFF }, 200 }, //
    { { 0x4C, 0x4C, 0x8C, 0xFF }, 400 }, //
    { { 0x5B, 0x5B, 0x9B, 0xFF }, 300 }, //
    { { 0x6A, 0x6A, 0xAA, 0xFF }, 200 }, //
    { { 0x79, 0x79, 0xB9, 0xFF }, 500 }, //
    { { 0x88, 0x88, 0xC8, 0xFF }, 350 }, //
    { { 0x97, 0x97, 0xD7, 0xFF }, 200 }, //
    { { 0xa6, 0xa6, 0xe6, 0xFF }, 250 }, //
};
static const size_t menu_bar_element_count = sizeof(menu_bar_elements)
    / sizeof(menu_bar_elements[0]);

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
        Clay_PointerData pointer = Clay_GetPointerState();

        for (size_t i = 0; i < menu_bar_element_count; i++) {
            MenuBarElementData element_data = menu_bar_elements[i];
            CLAY(CLAY_IDI("menu_bar_element", i), {
                    .layout = {
                        .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_FIXED(element_data.width), },
                    },
                    .backgroundColor = element_data.color,
                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                })
            {
                if (Clay_Hovered() && pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
                    dragged_menu_bar_element = i;
                }
            }
        }
    }

    Clay_ElementId menu_bar_id = Clay_GetElementId(CLAY_STRING("menu_bar"));
    Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(menu_bar_id);
    if (scrollData.found) {
        CLAY(CLAY_ID("ScrollBar"), {
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                    .offset = {.x = -(scrollData.scrollPosition->x / scrollData.contentDimensions.width) * scrollData.scrollContainerDimensions.width},
                    .zIndex = 1,
                    .parentId = menu_bar_id.id,
                    .attachPoints = { .element = CLAY_ATTACH_POINT_LEFT_BOTTOM, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM },
                },
            })
        {
            CLAY(CLAY_ID("ScrollBarButton"), {
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED((scrollData.scrollContainerDimensions.width / scrollData.contentDimensions.width) * scrollData.scrollContainerDimensions.width),
                            .height = CLAY_SIZING_FIXED(12),
                        }
                    },
                    .backgroundColor = Clay_PointerOver(Clay_GetElementId(CLAY_STRING("ScrollBar"))) && dragged_menu_bar_element == (size_t)-1 ? (Clay_Color){100, 100, 140, 150} : (Clay_Color){120, 120, 160, 150},
                    .cornerRadius = CLAY_CORNER_RADIUS(6),
                })
            {
            }
        }
    }

    CLAY(CLAY_ID("menu_bar2"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_GROW(0), },
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = CLAY_CORNER_RADIUS(8),
        .clip = { .horizontal = true, .childOffset = Clay_GetScrollOffset() },
    })
    {
        for (size_t i = 0; i < menu_bar_element_count; i++) {
            MenuBarElementData element_data = menu_bar_elements[i];
            CLAY(CLAY_IDI("menu_bar2_element", i), {
                    .layout = {
                        .sizing = {
                            .height = CLAY_SIZING_FIXED(60),
                            .width = CLAY_SIZING_FIXED(element_data.width),
                        },
                    },
                    .backgroundColor = element_data.color,
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

void dragged_menu_bar_element_layout(void)
{
    MenuBarElementData element_data = menu_bar_elements[dragged_menu_bar_element];
    Clay_PointerData pointer = Clay_GetPointerState();
    CLAY(
            CLAY_ID("dragged_menu_bar_element"),
            {
                .floating = {
                    .offset = {.x = pointer.position.x, .y = pointer.position.y},
                    .attachTo = CLAY_ATTACH_TO_ROOT,
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_TOP,
                        .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                    },
                    .zIndex = 1,
                },
            })
    {
        CLAY(CLAY_ID("dragged_menu_bar_element_inner"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(element_data.width),
                    .height = CLAY_SIZING_FIXED(60),
                },
            },
            .backgroundColor = element_data.color,
            .cornerRadius = CLAY_CORNER_RADIUS(8),
        })
        {
        }
    }

    // Display a white vertical line at the nearest side of the element under the pointer
    Clay_ElementData bar_element_data = Clay_GetElementData(CLAY_ID("menu_bar"));
    float bar_x = bar_element_data.found ? bar_element_data.boundingBox.x : 0.f;
    float pointer_x = pointer.position.x;
    float line_x = 0.f;
    size_t tab_i = 0;
    for (; tab_i < menu_bar_element_count; tab_i++) {
        Clay_ElementData elementData = Clay_GetElementData(CLAY_IDI("menu_bar_element", tab_i));
        if (!elementData.found) {
            continue;
        }
        float left = elementData.boundingBox.x;
        float right = left + elementData.boundingBox.width;
        float mid = left + elementData.boundingBox.width / 2.f;
        if (pointer_x >= left && pointer_x < right) {
            if (pointer_x < mid) {
                line_x = left;
            } else {
                line_x = right;
                tab_i++;
            }
            line_x -= bar_x;
            break;
        }
    }
    CLAY(CLAY_ID("drop_indicator"), {
            .floating = {
                .offset = { .x = line_x, .y = 0 },
                .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                .parentId = Clay_GetElementId(CLAY_STRING("menu_bar")).id,
                .attachPoints = {
                    .element = CLAY_ATTACH_POINT_LEFT_TOP,
                    .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                },
                .zIndex = 1,
            },
        })
    {
        CLAY(CLAY_ID("drop_indicator_line"), {
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(2),
                        .height = CLAY_SIZING_FIXED(60),
                    },
                },
                .backgroundColor = (Clay_Color){255, 255, 255, 255},
            })
        {
        }
    }
    if (pointer.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
        MenuBarElementData temp = { 0 };
        temp = menu_bar_elements[dragged_menu_bar_element];
        menu_bar_elements[dragged_menu_bar_element] = menu_bar_elements[tab_i];
        menu_bar_elements[tab_i] = temp;
        dragged_menu_bar_element = (size_t)-1;
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

    if (dragged_menu_bar_element != (size_t)-1) {
        dragged_menu_bar_element_layout();
    }

    return Clay_EndLayout(0);
}

void HandleClayErrors(Clay_ErrorData errorData)
{
    printf("%s", errorData.errorText.chars);
}

typedef struct {
    float sensitivity;
    bool use_both_wheels;
} ScrollContainerData;

// Apply wheel scroll to a specific element with a custom sensitivity and potential axis
// combination. Clay internally multiplies delta by 10, so we replicate that here, sensitivity = 1.0
// matches Clay's default speed. Returns true if the mouse was over the element and the scroll was
// applied.
static bool clay_scroll_apply(
    Clay_ElementId id,
    Clay_Vector2 mouse_pos,
    Clay_Vector2 delta,
    ScrollContainerData scroll_container_data)
{
    if (delta.x == 0 && delta.y == 0 && dragged_menu_bar_element == (size_t)-1) {
        return false;
    }

    Clay_ElementData elem = Clay_GetElementData(id);
    if (!elem.found) {
        return false;
    }
    Clay_BoundingBox bb = elem.boundingBox;
    if (!Clay__PointIsInsideRect(mouse_pos, bb)) {
        return false;
    }

    Clay_ScrollContainerData scroll = Clay_GetScrollContainerData(id);
    if (!scroll.found) {
        return false;
    }

    if (scroll_container_data.use_both_wheels) {
        assert(
            (scroll.config.horizontal != scroll.config.vertical)
            && "Exactly one of horizontal/vertical scrolling should be enabled when "
               "use_both_wheels is true");
        if (scroll.config.horizontal) {
            delta.x += delta.y;
        } else if (scroll.config.vertical) {
            delta.y += delta.x;
        }
    }

    if (dragged_menu_bar_element != (size_t)-1) {
#define DRAG_SCROLL_MARGIN 48
#define DRAG_SCROLL_SPEED 0.1f
        // If dragging, apply additional scroll when close to the edges to allow dragging beyond the
        // current view. The closer to the edge, the faster the scroll.
        float distance_to_left_edge = mouse_pos.x - bb.x;
        float distance_to_right_edge = (bb.x + bb.width) - mouse_pos.x;
        if (distance_to_left_edge < DRAG_SCROLL_MARGIN) {
            delta.x += (DRAG_SCROLL_MARGIN - distance_to_left_edge) * DRAG_SCROLL_SPEED
                / distance_to_left_edge;
        } else if (distance_to_right_edge < DRAG_SCROLL_MARGIN) {
            delta.x -= (DRAG_SCROLL_MARGIN - distance_to_right_edge) * DRAG_SCROLL_SPEED
                / distance_to_right_edge;
        }
    }

    const float scale = scroll_container_data.sensitivity * 10.0f;

    if (scroll.config.horizontal) {
        scroll.scrollPosition->x += delta.x * scale;
        float min_x = -(
            SDL_max(scroll.contentDimensions.width - scroll.scrollContainerDimensions.width, 0));
        scroll.scrollPosition->x = SDL_clamp(scroll.scrollPosition->x, min_x, 0);
    }
    if (scroll.config.vertical) {
        scroll.scrollPosition->y += delta.y * scale;
        float min_y = -(
            SDL_max(scroll.contentDimensions.height - scroll.scrollContainerDimensions.height, 0));
        scroll.scrollPosition->y = SDL_clamp(scroll.scrollPosition->y, min_y, 0);
    }
    return true;
}

typedef struct {
    Clay_Vector2 clickOrigin;
    Clay_Vector2 positionOrigin;
    bool mouseDown;
} ScrollbarData;

ScrollbarData scrollbarData = { 0 };

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
        Clay_Vector2 scroll_delta = { 0, 0 };
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
                    scroll_delta.x += event.wheel.x;
                    scroll_delta.y += event.wheel.y;
                    break;
                case SDL_EVENT_KEY_DOWN: break;
                case SDL_EVENT_TEXT_INPUT: break;
                case SDL_EVENT_KEYMAP_CHANGED: break;
                default: break;
            }
        }

        float mouse_x, mouse_y;
        Uint32 buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        Clay_Vector2 mouse_pos = { mouse_x, mouse_y };

        if (!(buttons & SDL_BUTTON_LMASK)) {
            scrollbarData.mouseDown = false;
        }
        if ((buttons & SDL_BUTTON_LMASK) && !scrollbarData.mouseDown
            && Clay_PointerOver(Clay_GetElementId(CLAY_STRING("ScrollBar")))
            && dragged_menu_bar_element == (size_t)-1) {
            Clay_ScrollContainerData scrollContainerData = Clay_GetScrollContainerData(
                Clay_GetElementId(CLAY_STRING("menu_bar")));
            scrollbarData.clickOrigin = mouse_pos;
            scrollbarData.positionOrigin = *scrollContainerData.scrollPosition;
            scrollbarData.mouseDown = true;
        } else if (scrollbarData.mouseDown) {
            Clay_ScrollContainerData scrollContainerData = Clay_GetScrollContainerData(
                Clay_GetElementId(CLAY_STRING("menu_bar")));
            if (scrollContainerData.contentDimensions.height > 0) {
                Clay_Vector2 ratio = (Clay_Vector2) {
                    scrollContainerData.contentDimensions.width
                        / scrollContainerData.scrollContainerDimensions.width,
                    scrollContainerData.contentDimensions.height
                        / scrollContainerData.scrollContainerDimensions.height,
                };
                if (scrollContainerData.config.vertical) {
                    scrollContainerData.scrollPosition->y = scrollbarData.positionOrigin.y
                        + (scrollbarData.clickOrigin.y - mouse_pos.y) * ratio.y;
                }
                if (scrollContainerData.config.horizontal) {
                    scrollContainerData.scrollPosition->x = scrollbarData.positionOrigin.x
                        + (scrollbarData.clickOrigin.x - mouse_pos.x) * ratio.x;
                }
            }
        }

        // Custom scroll handling for the menu bar with a different sensitivity and both axes
        // enabled
        bool scroll_consumed = clay_scroll_apply(
            CLAY_ID("menu_bar"),
            mouse_pos,
            scroll_delta,
            (ScrollContainerData) {
                .sensitivity = 5.f,
                .use_both_wheels = true,
            });
        // Pass {0,0} if already handled above (preserves drag/momentum for other areas), or the
        // real delta as the default fallback
        Clay_UpdateScrollContainers(
            false,
            scroll_consumed ? (Clay_Vector2) { 0, 0 } : scroll_delta,
            0.016f);

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

/*
TODO:

1. tab bar
- [x] implement the dropping of tabs (just swap entries)
- [ ] add clipping to the scrollable area to avoid having the drop indicator drawn outside
- [ ] start dragging only when clicking AND moving the mouse by a threshold
*/
