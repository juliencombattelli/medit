#include "clay_sdl3.h"

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

#define NO_DRAGGED_MENU_BAR_ELEMENT ((size_t) - 1)

static const Clay_Color background_color = { 0x18, 0x18, 0x18, 0xFF };
static const Clay_Color sidebars_background_color = { 0x18, 0x7f, 0x18, 0xFF };
static const Clay_Color editor_background_color = { 0x1F, 0x1F, 0x5F, 0xFF };
static const Clay_Color scrollbar_inactive_color = { 100, 100, 100, 150 };
static const Clay_Color scrollbar_hovered_color = { 120, 120, 120, 150 };
static const Clay_Color scrollbar_active_color = { 140, 140, 140, 150 };

static const float drag_dead_zone_pixels = 4;

static size_t dragged_menu_bar_element = NO_DRAGGED_MENU_BAR_ELEMENT;
static MouseState mouse_state = { 0 };
static DragState drag_state = {
    .active_id = CLAY_EXT_NULL_ID,
};

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

Clay_Color get_scrollbar_color(void)
{
    if (drag_state.active_id == Clay_GetOpenElementId()) {
        return scrollbar_active_color;
    }
    if (!is_any_element_dragged(&drag_state) && Clay_Hovered()) {
        return scrollbar_hovered_color;
    }
    return scrollbar_inactive_color;
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
}

void dragged_menu_bar_element_drop_indicator_layout(void)
{
    const Clay_PointerData pointer = Clay_GetPointerState();

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
    if (line_x < 0 || line_x > bar_element_data.boundingBox.width) {
        return;
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
    if (mouse_state.buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_RELEASED_THIS_FRAME) {
        size_t src = dragged_menu_bar_element;
        size_t dst = tab_i;
        if (src != dst && src + 1 != dst) {
            MenuBarElementData elem = menu_bar_elements[src];
            if (src < dst) {
                // Moving right: close the gap by shifting [src+1 .. dst-1] left
                for (size_t i = src; i < dst - 1; i++) {
                    menu_bar_elements[i] = menu_bar_elements[i + 1];
                }
                menu_bar_elements[dst - 1] = elem;
            } else {
                // Moving left: open a slot by shifting [dst .. src-1] right
                for (size_t i = src; i > dst; i--) {
                    menu_bar_elements[i] = menu_bar_elements[i - 1];
                }
                menu_bar_elements[dst] = elem;
            }
        }
    }
}

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
                if (Clay_Hovered() && !is_any_element_dragged(&drag_state)
                    && dragged_menu_bar_element == NO_DRAGGED_MENU_BAR_ELEMENT
                    && mouse_state.buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_PRESSED) {
                    float distance_from_click_origin = distance(
                        mouse_state.buttons[MOUSE_BUTTON_LEFT].click_origin,
                        mouse_state.pos);
                    if (distance_from_click_origin > drag_dead_zone_pixels) {
                        dragged_menu_bar_element = i;
                        drag_state.active_id = Clay_GetElementId(
                                                   CLAY_STRING("dragged_menu_bar_element"))
                                                   .id;
                        drag_state.click_origin = mouse_state.buttons[MOUSE_BUTTON_LEFT]
                                                      .click_origin;
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
                    .backgroundColor = get_scrollbar_color(),
                    .cornerRadius = CLAY_CORNER_RADIUS(6),
                })
                {
                }
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

    Clay_ElementId dragged_menu_bar_element_id = Clay_GetElementId(
        CLAY_STRING("dragged_menu_bar_element"));

    if (dragged_menu_bar_element != NO_DRAGGED_MENU_BAR_ELEMENT
        && drag_state.active_id == dragged_menu_bar_element_id.id) {
        dragged_menu_bar_element_layout();
        Clay_ElementData bar_elem = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("menu_bar")));
        Clay_PointerData ptr = Clay_GetPointerState();
        bool inside = bar_elem.found && point_inside_rect(ptr.position, bar_elem.boundingBox);
        if (inside) {
            dragged_menu_bar_element_drop_indicator_layout();
        }
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
        mouse_state_reset(&mouse_state);
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
                    mouse_state.scroll_delta.x += event.wheel.x;
                    mouse_state.scroll_delta.y += event.wheel.y;
                    break;
                case SDL_EVENT_KEY_DOWN: break;
                case SDL_EVENT_TEXT_INPUT: break;
                case SDL_EVENT_KEYMAP_CHANGED: break;
                default: break;
            }
        }

        mouse_state_update(&mouse_state, sdl_get_mouse_state(&mouse_state));

        Clay_SetPointerState(mouse_state.pos, mouse_state.buttons[MOUSE_BUTTON_LEFT].state);

        bool scroll_consumed = Clay_Ext_UpdateScrollContainerCustom(
            SCROLL_UPDATE_SOURCE_WHEEL | SCROLL_UPDATE_SOURCE_SCROLLBAR,
            CLAY_ID("menu_bar"),
            CLAY_ID("ScrollBarButton"),
            mouse_state.pos,
            mouse_state.scroll_delta,
            (ScrollContainerData) {
                .sensitivity = 5.f,
                .use_both_wheels = true,
            },
            &mouse_state,
            &drag_state);

        // Pass {0,0} if already handled above (preserves drag/momentum for other areas), or the
        // real delta as the default fallback
        Clay_UpdateScrollContainers(
            false,
            scroll_consumed ? (Clay_Vector2) { 0 } : mouse_state.scroll_delta,
            0.016f);

        Clay_RenderCommandArray render_commands = editor_layout();

        SDL_SetRenderDrawColor(state.renderer, 0, 128, 0, 255);
        SDL_RenderClear(state.renderer);

        SDL_Clay_RenderClayCommands(state.renderer, render_commands);

        SDL_RenderPresent(state.renderer);

        // Ensure all drag states are reset when the mouse key is released
        if (mouse_state.buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_RELEASED_THIS_FRAME) {
            dragged_menu_bar_element = NO_DRAGGED_MENU_BAR_ELEMENT;
            drag_state.active_id = CLAY_EXT_NULL_ID;
        }
    }

    SDL_StopTextInput(state.window);

    TTF_DestroyRendererTextEngine(state.text_engine);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

    TTF_Quit();
    SDL_Quit();
}
