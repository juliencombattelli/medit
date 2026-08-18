#include "ui.h"

bool medit_ui_is_any_element_dragged(const Ui* ui)
{
    return ui->drag_state.active_id != CLAY_EXT_NULL_ID;
}

void medit_ui_update_scroll_containers(Ui* ui)
{
    for (size_t i = 0; i < ui->scroll_container_count; i++) {
        const ScrollContainerCustom* scroll_container = &ui->scroll_containers[i];
        const Clay_ElementId container_id = CLAY_SID(Clay_Ext_StringFromCStr(scroll_container->container_id));
        const Clay_ElementId scrollbar_h_button_id = Clay__HashString(CLAY_STRING("scrollbar_h_button"), container_id.id);
        const Clay_ElementId scrollbar_v_button_id = Clay__HashString(CLAY_STRING("scrollbar_v_button"), container_id.id);
        Clay_Ext_UpdateScrollContainerCustom(
            container_id,
            scrollbar_h_button_id,
            scrollbar_v_button_id,
            ui->mouse_state.pos,
            ui->mouse_state.scroll_delta,
            (ScrollContainerConfig) {
                .sensitivity_h = 30.f,
                .sensitivity_v = 10.f,
                .enable_drag_on_edges = true,
            },
            &ui->mouse_state,
            &ui->drag_state);
    }

    // Handle scroll delta with mouse wheels only for other scrollable areas
    Clay_UpdateScrollContainers(
        false, // do not enable touch and drag scrolling as it is handled per scrollable area
        ui->mouse_state.scroll_delta,
        0.f); // not used when touch and drag scrolling is disabled
}

static Clay_Color get_scrollbar_color(const Ui* ui)
{
    if (ui->drag_state.active_id == Clay_GetOpenElementId()) {
        return to_clay_color(ui->theme.colors.scrollbar_thumb_active);
    }
    if (!medit_ui_is_any_element_dragged(ui) && Clay_Hovered()) {
        return to_clay_color(ui->theme.colors.scrollbar_thumb_hovered);
    }
    return to_clay_color(ui->theme.colors.scrollbar_thumb_inactive);
}

// Lay out a scrollbar with predefined settings for the opened element
void medit_ui_layout_scrollbar(Ui* ui)
{
    // TODO verify that only one scrollbar is displayed when overflowing on only one dimension
    const float scrollbar_size = (float)ui->theme.scrollbar_size;
    const float scrollbar_corner_radius = (float)ui->theme.scrollbar_corner_radius;

    uint32_t parent_id = Clay_GetOpenElementId();
    Clay_ScrollContainerData scroll_data = Clay_GetScrollContainerData((Clay_ElementId){ .id = parent_id });
    if (scroll_data.found) {
        if (scroll_data.config.horizontal) {
            const Clay_ElementId outer_id = CLAY_ID_LOCAL("scrollbar_h_outer");
            const Clay_ElementId button_id = CLAY_ID_LOCAL("scrollbar_h_button");
            CLAY(outer_id, {
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                    .offset = { .x =
                        -(scroll_data.scrollPosition->x / scroll_data.contentDimensions.width)
                        * scroll_data.scrollContainerDimensions.width
                    },
                    .zIndex = 1,
                    .parentId = parent_id,
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                        .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                    },
                },
            }) {
                CLAY(button_id, {
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(
                                ((scroll_data.scrollContainerDimensions.width / scroll_data.contentDimensions.width)
                                * scroll_data.scrollContainerDimensions.width)
                                - (scroll_data.config.vertical ? scrollbar_size : 0)),
                            .height = CLAY_SIZING_FIXED(scrollbar_size),
                        }
                    },
                    .backgroundColor = get_scrollbar_color(ui),
                    .cornerRadius = CLAY_CORNER_RADIUS(scrollbar_corner_radius),
                });
            }
        }
        if (scroll_data.config.vertical) {
            const Clay_ElementId outer_id = CLAY_ID_LOCAL("scrollbar_v_outer");
            const Clay_ElementId button_id = CLAY_ID_LOCAL("scrollbar_v_button");
            CLAY(outer_id, {
                .floating = {
                    .attachTo = CLAY_ATTACH_TO_ELEMENT_WITH_ID,
                    .offset = { .y =
                        -(scroll_data.scrollPosition->y / scroll_data.contentDimensions.height)
                        * scroll_data.scrollContainerDimensions.height
                    },
                    .zIndex = 1,
                    .parentId = parent_id,
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_RIGHT_TOP,
                        .parent = CLAY_ATTACH_POINT_RIGHT_TOP,
                    },
                },
            }) {
                CLAY(button_id, {
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_FIXED(scrollbar_size),
                            .height = CLAY_SIZING_FIXED(
                                (scroll_data.scrollContainerDimensions.height / scroll_data.contentDimensions.height)
                                * scroll_data.scrollContainerDimensions.height),
                        }
                    },
                    .backgroundColor = get_scrollbar_color(ui),
                    .cornerRadius = CLAY_CORNER_RADIUS(scrollbar_corner_radius),
                });
            }
        }
    }
}
