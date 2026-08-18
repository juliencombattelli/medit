#include "clay_sdl3.h"
#include "titlebar.h"
#include "ui.h"

#include <core/assert.h>

#include <default_config.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
} AppState;

#define TITLE_BAR_HEIGHT 32
#define TITLE_BAR_BUTTONS_WIDTH 46
#define WINDOW_HIT_TEST_MARGIN 4

#define NO_DRAGGED_MENU_BAR_ELEMENT ((size_t)-1)

static const Clay_Color background_color            = { 0x18, 0x18, 0x18, 0xFF };
static const Clay_Color sidebars_background_color   = { 0x18, 0x7f, 0x18, 0xFF };
static const Clay_Color editor_background_color     = { 0x1F, 0x1F, 0x5F, 0xFF };
static const Clay_Color drop_indicator_color        = { 0xFF, 0xFF, 0xFF, 0x9F };

static const uint8_t dragged_tab_transparency = 0x9F;

static const float drag_dead_zone_pixels = 4;

static size_t dragged_menu_bar_element = NO_DRAGGED_MENU_BAR_ELEMENT;

typedef struct {
    Clay_Color color;
    float width;
} MenuBarElementData;

static MenuBarElementData menu_bar_elements[] = {
    { .color = { 0x1F, 0x1F, 0x5F, 0xFF }, .width = 200 },
    { .color = { 0x2E, 0x2E, 0x6E, 0xFF }, .width = 250 },
    { .color = { 0x3D, 0x3D, 0x7D, 0xFF }, .width = 200 },
    { .color = { 0x4C, 0x4C, 0x8C, 0xFF }, .width = 400 },
    { .color = { 0x5B, 0x5B, 0x9B, 0xFF }, .width = 300 },
    { .color = { 0x6A, 0x6A, 0xAA, 0xFF }, .width = 200 },
    { .color = { 0x79, 0x79, 0xB9, 0xFF }, .width = 500 },
    { .color = { 0x88, 0x88, 0xC8, 0xFF }, .width = 350 },
    { .color = { 0x97, 0x97, 0xD7, 0xFF }, .width = 200 },
    { .color = { 0xa6, 0xa6, 0xe6, 0xFF }, .width = 250 },
};

static const size_t menu_bar_element_count = sizeof(menu_bar_elements) / sizeof(menu_bar_elements[0]);

static const ScrollContainerCustom scroll_container_data_array[] = {
    {
        .container_id = "menu_bar",
        .config = {
            .sensitivity_h = 30.f,
            .sensitivity_v = 10.f,
            .enable_drag_on_edges = true,
        },
    },
    {
        .container_id = "menu_bar2",
        .config = {
            .sensitivity_h = 30.f,
            .sensitivity_v = 10.f,
            .use_both_wheels = true,
        },
    },
};

static const size_t scroll_container_data_count = sizeof(scroll_container_data_array) / sizeof(scroll_container_data_array[0]);

static void dragged_menu_bar_element_layout(Ui* ui)
{
    MenuBarElementData element_data = menu_bar_elements[dragged_menu_bar_element];
    Clay_PointerData pointer = Clay_GetPointerState();
    Clay_Color color = element_data.color;
    color.a = dragged_tab_transparency;
    CLAY(CLAY_ID("dragged_menu_bar_element"), {
        .floating = {
            .offset = {.x = pointer.position.x, .y = pointer.position.y},
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_LEFT_TOP,
                .parent = CLAY_ATTACH_POINT_LEFT_TOP,
            },
            .zIndex = 1,
        },
    }) {
        CLAY(CLAY_ID("dragged_menu_bar_element_inner"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(element_data.width),
                    .height = CLAY_SIZING_FIXED(60),
                },
            },
            .backgroundColor = color,
            .cornerRadius = ui->theme.panels_corner_radius,
        });
    }
}

static void dragged_menu_bar_element_drop_indicator_layout(Ui* ui)
{
    // Compute the destination tab index and line position
    const Clay_PointerData pointer = Clay_GetPointerState();
    const Clay_ElementData bar_element_data = Clay_GetElementData(CLAY_ID("menu_bar"));
    const float bar_x = bar_element_data.found ? bar_element_data.boundingBox.x : 0.f;
    const float pointer_x = pointer.position.x;
    float line_x = 0.f;
    uint32_t tab_i = 0;
    for (; tab_i < menu_bar_element_count; tab_i++) {
        Clay_ElementData element_data = Clay_GetElementData(CLAY_IDI("menu_bar_element", tab_i));
        if (!element_data.found) {
            continue;
        }
        float left = element_data.boundingBox.x;
        float right = left + element_data.boundingBox.width;
        float mid = left + (element_data.boundingBox.width / 2.f);
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

    // If the line is outside of the container don't draw it and don't handle dropping
    if (line_x < 0 || line_x > bar_element_data.boundingBox.width) {
        return;
    }

    // Lay out the drop line indicator
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
    }) {
        CLAY(CLAY_ID("drop_indicator_line"), {
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(2),
                    .height = CLAY_SIZING_FIXED(60),
                },
            },
            .backgroundColor = drop_indicator_color,
        });
    }

    // Drop the dragged tab into the choosen position on mouse button release
    if (ui->mouse_state.buttons[MOUSE_BUTTON_LEFT].state == MOUSE_BUTTON_RELEASED_THIS_FRAME) {
        size_t src = dragged_menu_bar_element;
        size_t dst = tab_i;
        MenuBarElementData elem = menu_bar_elements[src];
        if (src < dst) {
            memmove(&menu_bar_elements[src], &menu_bar_elements[src + 1], (dst - src - 1) * sizeof(elem));
            menu_bar_elements[dst - 1] = elem;
        } else if (src > dst) {
            memmove(&menu_bar_elements[dst + 1], &menu_bar_elements[dst], (src - dst) * sizeof(elem));
            menu_bar_elements[dst] = elem;
        }
    }
}

static void handle_menu_bar_element_interaction(Clay_ElementId element_id, Clay_PointerData pointer, void *userdata)
{
    (void)pointer;

    Ui* ui = (Ui*)userdata;
    uint32_t i = element_id.offset & 0xFF;

    Clay_ElementData bar_elem = Clay_GetElementData(element_id);
    bool clicked = bar_elem.found && point_inside_rect(ui->mouse_state.buttons[MOUSE_BUTTON_LEFT].click_origin, bar_elem.boundingBox);

    if (clicked
        && !is_any_element_dragged(&ui->drag_state)
        && dragged_menu_bar_element == NO_DRAGGED_MENU_BAR_ELEMENT)
    {
        float distance_from_click_origin = distance(
            ui->mouse_state.buttons[MOUSE_BUTTON_LEFT].click_origin,
            ui->mouse_state.pos);
        if (distance_from_click_origin > drag_dead_zone_pixels) {
            dragged_menu_bar_element = i;
            ui->drag_state.active_id = Clay_GetElementId(CLAY_STRING("dragged_menu_bar_element")).id;
            ui->drag_state.is_droppable = true;
            ui->drag_state.click_origin = ui->mouse_state.buttons[MOUSE_BUTTON_LEFT].click_origin;
        }
    }
}

static void menu_bar_layout(Ui* ui)
{
    CLAY(CLAY_ID("menu_bar"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = { .height = CLAY_SIZING_FIXED(300), .width = CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(8),
            .childGap = 8
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
        .clip = { .horizontal = true, .vertical = true, .childOffset = Clay_GetScrollOffset() },
    }) {
        for (uint32_t row = 0; row < 8; row++) {
            CLAY(CLAY_IDI("menu_bar_row", row), {
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childGap = 8,
                },
            }) {
                for (uint32_t i = 0; i < menu_bar_element_count; i++) {
                    MenuBarElementData element_data = menu_bar_elements[i];
                    CLAY(CLAY_IDI("menu_bar_element", (row << 8) + i), {
                        .layout = {
                            .sizing = {
                                .height = CLAY_SIZING_FIXED(100),
                                .width = CLAY_SIZING_FIXED(element_data.width),
                            },
                        },
                        .backgroundColor = element_data.color,
                        .cornerRadius = ui->theme.panels_corner_radius,
                    }) {
                        Clay_OnHover(handle_menu_bar_element_interaction, ui);
                    }
                }
            }
        }

        medit_ui_layout_scrollbar(ui);
    }

    CLAY(CLAY_ID("menu_bar2"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_GROW(0) },
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
        .clip = { .horizontal = true, .childOffset = Clay_GetScrollOffset() },
    }) {
        for (uint32_t i = 0; i < menu_bar_element_count; i++) {
            MenuBarElementData element_data = menu_bar_elements[i];
            CLAY(CLAY_IDI("menu_bar2_element", i), {
                .layout = {
                    .sizing = {
                        .height = CLAY_SIZING_FIXED(60),
                        .width = CLAY_SIZING_FIXED(element_data.width),
                    },
                },
                .backgroundColor = element_data.color,
                .cornerRadius = ui->theme.panels_corner_radius,
            });
        }

        medit_ui_layout_scrollbar(ui);
    }

    CLAY(CLAY_ID("menu_bar3"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_FIXED(60), .width = CLAY_SIZING_GROW(0) },
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
        .clip = { .horizontal = true, .childOffset = Clay_GetScrollOffset() },
    }) {
        for (uint32_t i = 0; i < menu_bar_element_count; i++) {
            MenuBarElementData element_data = menu_bar_elements[i];
            CLAY(CLAY_IDI("menu_bar3_element", i), {
                .layout = {
                    .sizing = {
                        .height = CLAY_SIZING_FIXED(60),
                        .width = CLAY_SIZING_FIXED(element_data.width),
                    },
                },
                .backgroundColor = element_data.color,
                .cornerRadius = ui->theme.panels_corner_radius,
            });
        }
    }
}

static void left_panel_layout(Ui* ui)
{
    CLAY(CLAY_ID("left_panel"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_FIXED(100) },
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
    });
}

static void editor_area_layout(Ui* ui)
{
    CLAY(CLAY_ID("editor_area"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0) },
        },
        .backgroundColor = editor_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
    });
}

static void right_panel_layout(Ui* ui)
{
    CLAY(CLAY_ID("right_panel"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_FIXED(100) },
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
    });
}

static void middle_area_layout(Ui* ui)
{
    CLAY(CLAY_ID("middle_area"), {
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { .height = CLAY_SIZING_GROW(0), .width = CLAY_SIZING_GROW(0) },
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16,
        },
        .backgroundColor = background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
    }) {
        left_panel_layout(ui);
        editor_area_layout(ui);
        right_panel_layout(ui);
    }
}

static void status_bar_layout(Ui* ui)
{
    CLAY(CLAY_ID("status_bar"), {
        .layout = {
            .sizing = { .height = CLAY_SIZING_FIXED(30), .width = CLAY_SIZING_GROW(0) },
        },
        .backgroundColor = sidebars_background_color,
        .cornerRadius = ui->theme.panels_corner_radius,
    });
}

static Clay_RenderCommandArray editor_layout(Ui* ui)
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
        titlebar_layout(&ui->titlebar_state);

        CLAY(CLAY_ID("window_frame_inner"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0),
                },
                .padding = CLAY_PADDING_ALL(16),
                .childGap = ui->theme.panels_gap,
            },
            .backgroundColor = { 0x7F, 0x00, 0x00, 0xFF},
        }) {
            menu_bar_layout(ui);
            middle_area_layout(ui);
            status_bar_layout(ui);
        }
    }

    Clay_ElementId dragged_menu_bar_element_id = Clay_GetElementId(CLAY_STRING("dragged_menu_bar_element"));

    if (dragged_menu_bar_element != NO_DRAGGED_MENU_BAR_ELEMENT
        && ui->drag_state.active_id == dragged_menu_bar_element_id.id)
    {
        Clay_ElementData bar_elem = Clay_GetElementData(Clay_GetElementId(CLAY_STRING("menu_bar")));
        Clay_PointerData ptr = Clay_GetPointerState();
        bool inside = bar_elem.found && point_inside_rect(ptr.position, bar_elem.boundingBox);
        if (inside) {
            dragged_menu_bar_element_drop_indicator_layout(ui);
        }
        dragged_menu_bar_element_layout(ui);
    }

    return Clay_EndLayout(0);
}

static void handle_clay_errors(Clay_ErrorData error_data)
{
    printf("%s\n", error_data.errorText.chars);
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    AppState state = { 0 };

    Ui ui = {
        .scroll_containers = scroll_container_data_array,
        .scroll_container_count = scroll_container_data_count,
        .titlebar_state = {
            .height = TITLE_BAR_HEIGHT,
            .resize_border = WINDOW_HIT_TEST_MARGIN,
            .button_width = TITLE_BAR_BUTTONS_WIDTH,
        },
        .theme = {
            .colors = {
                .scrollbar_thumb_scroll_area_hovered = { 120 + 100, 120, 120, 150 },
                .scrollbar_thumb_hovered = { 140 + 100, 140, 140, 150 },
                .scrollbar_thumb = { 100 + 100, 100, 100, 150 },
            },
            .scrollbar_corner_radius = 6,
            .scrollbar_size = 12,
            .dragged_tab_transparency = 0x9F,
            .panels_gap = 8,
            .panels_corner_radius = CLAY_CORNER_RADIUS(8),
        },
    };

    assert(SDL_Init(SDL_INIT_VIDEO));
    assert(TTF_Init());

    SDL_WindowFlags window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY;

#if SDL_PLATFORM_LINUX
    const char* is_running_on_wslg = SDL_getenv("WSL2_GUI_APPS_ENABLED");
    if (is_running_on_wslg && SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        // When running in WSLg using X11, borderless apps show a weird offset from top-left screen corner when
        // maximized if fractional scaling is used. So we force classic windowed mode with borders.
        window_flags &= ~SDL_WINDOW_BORDERLESS;
    }
#endif

    state.window = SDL_CreateWindow("UI Playground", 1280, 720, window_flags);
    assert_sdl(state.window);

    state.renderer = SDL_CreateRenderer(state.window, NULL);
    assert_sdl(state.renderer);

    assert_sdl(SDL_SetRenderVSync(state.renderer, 1));

    state.text_engine = TTF_CreateRendererTextEngine(state.renderer);
    assert_sdl(state.text_engine);

    assert_sdl(SDL_ShowWindow(state.window));
    assert_sdl(SDL_StartTextInput(state.window));

    titlebar_init(&ui.titlebar_state, state.window);

    {
        int width = 0, height = 0;
        SDL_GetWindowSize(state.window, &width, &height);
        uint32_t mem_size = Clay_MinMemorySize();
        Clay_Initialize(
            Clay_CreateArenaWithCapacityAndMemory(mem_size, malloc(mem_size)),
            (Clay_Dimensions) { .width = (float)width, .height = (float)height },
            (Clay_ErrorHandler) { .errorHandlerFunction = handle_clay_errors });
    }

    bool running = true;
    while (running) {
        SDL_Event event = { 0 };
        mouse_state_reset(&ui.mouse_state);
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_WINDOW_RESIZED: {
                    int width = event.window.data1;
                    int height = event.window.data2;
                    Clay_SetLayoutDimensions((Clay_Dimensions) { (float)width, (float)height });
                } break;
                case SDL_EVENT_MOUSE_WHEEL:
                    ui.mouse_state.scroll_delta.x += event.wheel.x;
                    ui.mouse_state.scroll_delta.y += event.wheel.y;
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_TEXT_INPUT:
                case SDL_EVENT_KEYMAP_CHANGED:
                default: break;
            }
        }

        mouse_state_update(&ui.mouse_state, sdl_get_mouse_state(state.window, &ui.mouse_state));

        Clay_SetPointerState(ui.mouse_state.pos, ui.mouse_state.buttons[MOUSE_BUTTON_LEFT].state);

        medit_ui_update_scroll_containers(&ui);

        titlebar_update(&ui.titlebar_state, state.window, &ui.mouse_state, &running);

        Clay_RenderCommandArray render_commands = editor_layout(&ui);

        SDL_SetRenderDrawColor(state.renderer, 0, 128, 0, 255);
        SDL_RenderClear(state.renderer);

        SDL_Clay_RenderClayCommands(state.renderer, render_commands);

        SDL_RenderPresent(state.renderer);

        // Ensure all drag states are reset when the mouse key is released
        const MouseButtonData mb_left = ui.mouse_state.buttons[MOUSE_BUTTON_LEFT];
        if (mb_left.state == MOUSE_BUTTON_RELEASED_THIS_FRAME || mb_left.state == MOUSE_BUTTON_RELEASED) {
            dragged_menu_bar_element = NO_DRAGGED_MENU_BAR_ELEMENT;
            ui.drag_state.active_id = CLAY_EXT_NULL_ID;
        }
    }

    SDL_StopTextInput(state.window);

    TTF_DestroyRendererTextEngine(state.text_engine);
    SDL_DestroyRenderer(state.renderer);
    SDL_DestroyWindow(state.window);

    TTF_Quit();
    SDL_Quit();
}
