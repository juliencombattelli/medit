#include "ui.h"
#include "safeint.h"

#include "dynarray.h"
#include "utils.h"

void ui_draw_cmd_list_init(UiDrawCmdList* draw_cmd_list)
{
    *draw_cmd_list = (UiDrawCmdList) { 0 };
}

void ui_draw_cmd_list_free(UiDrawCmdList* draw_cmd_list)
{
    dynarray_free(*draw_cmd_list);
}

void ui_draw_cmd_list_clear(UiDrawCmdList* draw_cmd_list)
{
    draw_cmd_list->count = 0;
}

static void draw_cmd_list_push(UiDrawCmdList* draw_cmd_list, UiDrawCmd draw_cmd)
{
    dynarray_append(draw_cmd_list, draw_cmd);
}

// Compute a widget state from an input and a widget rect
static UiWidgetState widget_state(const UiInputEvent* input, Rect rect)
{
    UiWidgetState state = 0;
    if (!rect_contains(rect, input->x, input->y)) {
        return state;
    }
    state |= UI_STATE_HOVERED;
    if (input->left_down) {
        state |= UI_STATE_PRESSED;
    }
    if (input->left_clicked) {
        state |= UI_STATE_CLICKED;
    }
    return state;
}

// Compute normalized thumb position (0..1) and thumb-to-track ratio from scroll state and track
// length
static void scrollbar_geometry(
    float offset,
    float content,
    float viewport,
    float track_len,
    float* out_pos,
    float* out_ratio)
{
    float ratio = (content > 0.0f) ? (viewport / content) : 1.0f;
    if (ratio > 1.0f) {
        ratio = 1.0f;
    }

    float max_offset = content - viewport;
    float pos = (max_offset > 0.0f) ? (offset / max_offset) : 0.0f;
    pos = medit_clampf(pos, 0.0f, 1.0f);

    *out_ratio = ratio;
    *out_pos = pos;
    (void)track_len;
}

void ui_panel(UiCtx* ctx, Rect rect, Color bg)
{
    UiDrawCmd panel = {
        .kind = UI_CMD_RECT_FILLED,
        .rect = rect,
        .color = bg,
    };
    draw_cmd_list_push(ctx->draw_list, panel);
}

void ui_panel_bordered(UiCtx* ctx, Rect rect, Color bg, Color border)
{
    UiDrawCmd fill = {
        .kind = UI_CMD_RECT_FILLED,
        .rect = rect,
        .color = bg,
    };
    draw_cmd_list_push(ctx->draw_list, fill);

    UiDrawCmd outline = {
        .kind = UI_CMD_RECT_OUTLINED,
        .rect = rect,
        .color = border,
        .outline_color = border,
    };
    draw_cmd_list_push(ctx->draw_list, outline);
}

UiWidgetState ui_button(
    UiCtx* ctx,
    Rect rect,
    Color bg,
    Color hover_bg,
    Color press_bg,
    const char* label,
    Color label_color)
{
    UiWidgetState state = widget_state(&ctx->input, rect);

    Color fill = bg;
    if (state & UI_STATE_PRESSED) {
        fill = press_bg;
    } else if (state & UI_STATE_HOVERED) {
        fill = hover_bg;
    }

    UiDrawCmd button_rect = {
        .kind = UI_CMD_RECT_FILLED,
        .rect = rect,
        .color = fill,
    };
    draw_cmd_list_push(ctx->draw_list, button_rect);

    if (label) {
        UiDrawCmd text_label = {
            .kind = UI_CMD_TEXT,
            .rect = rect,
            .color = label_color,
            .text = label,
        };
        draw_cmd_list_push(ctx->draw_list, text_label);
    }

    return state;
}

UiWidgetState ui_scrollbar_v(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color)
{
    float viewport_h = (float)track.h;
    float content_h = scroll_state->content_h;
    float max_offset = content_h - viewport_h;

    float thumb_ratio = 0.f;
    float scroll_pos = 0.f;
    scrollbar_geometry(
        scroll_state->offset_y,
        content_h,
        viewport_h,
        viewport_h,
        &scroll_pos,
        &thumb_ratio);

    // Thumb geometry in UI-dependent coordinates
    float thumb_h = thumb_ratio * viewport_h;
    float thumb_y = (float)track.y + (scroll_pos * (viewport_h - thumb_h));
    Rect thumb_rect = {
        .x = track.x,
        .y = float_to_i32(thumb_y),
        .w = track.w,
        .h = float_to_i32(thumb_h),
    };

    UiWidgetState state = 0;
    bool mouse_on_track = rect_contains(track, ctx->input.x, ctx->input.y);
    bool mouse_on_thumb = rect_contains(thumb_rect, ctx->input.x, ctx->input.y);

    if (mouse_on_track || mouse_on_thumb) {
        state |= UI_STATE_HOVERED;
    }
    if (ctx->input.left_down && (mouse_on_track || mouse_on_thumb || scroll_state->drag_active_v)) {
        state |= UI_STATE_PRESSED;
    }
    if (ctx->input.left_clicked && (mouse_on_track || mouse_on_thumb)) {
        state |= UI_STATE_CLICKED;
    }

    // Drag start: mouse pressed on the thumb this frame
    if (ctx->input.left_down && mouse_on_thumb && !scroll_state->drag_active_v
        && !ctx->input.left_clicked /* not a release frame */) {
        scroll_state->drag_active_v = true;
        scroll_state->drag_start_mouse = ctx->input.y;
        scroll_state->drag_start_offset = scroll_state->offset_y;
    }

    // Continue drag
    if (scroll_state->drag_active_v) {
        if (ctx->input.left_down) {
            float delta_px = (float)(ctx->input.y - scroll_state->drag_start_mouse);
            float track_usable = viewport_h - thumb_h;
            if (track_usable > 0.0f && max_offset > 0.0f) {
                float new_offset = scroll_state->drag_start_offset
                    + (delta_px * (max_offset / track_usable));
                scroll_state->offset_y = medit_clampf(new_offset, 0.0f, max_offset);
            }
        } else {
            // Button released - end drag
            scroll_state->drag_active_v = false;
        }
    } else if ((state & UI_STATE_PRESSED) && !mouse_on_thumb) {
        // Click on track outside thumb - jump
        float rel = ((float)ctx->input.y - (float)track.y - (thumb_h / 2.f))
            / (viewport_h - thumb_h);
        rel = medit_clampf(rel, 0.0f, 1.0f);
        if (max_offset > 0.0f) {
            scroll_state->offset_y = rel * max_offset;
        }
    }

    // Recompute for draw command after potential offset change
    scrollbar_geometry(
        scroll_state->offset_y,
        content_h,
        viewport_h,
        viewport_h,
        &scroll_pos,
        &thumb_ratio);

    UiDrawCmd scrollbar = {
        .kind = UI_CMD_SCROLLBAR,
        .rect = track,
        .color = track_color,
        .thumb_color = thumb_color,
        .scroll_pos = scroll_pos,
        .thumb_ratio = thumb_ratio,
    };
    draw_cmd_list_push(ctx->draw_list, scrollbar);

    return state;
}

UiWidgetState ui_scrollbar_h(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color)
{
    float viewport_w = (float)track.w;
    float content_w = scroll_state->content_w;
    float max_offset = content_w - viewport_w;

    float thumb_ratio = 0.f;
    float scroll_pos = 0.f;
    scrollbar_geometry(
        scroll_state->offset_x,
        content_w,
        viewport_w,
        viewport_w,
        &scroll_pos,
        &thumb_ratio);

    // Thumb geometry in UI-dependent coordinates
    float thumb_w = thumb_ratio * viewport_w;
    float thumb_x = (float)track.x + (scroll_pos * (viewport_w - thumb_w));
    Rect thumb_rect = {
        .x = float_to_i32(thumb_x),
        .y = track.y,
        .w = float_to_i32(thumb_w),
        .h = track.h,
    };

    UiWidgetState state = 0;
    bool mouse_on_track = rect_contains(track, ctx->input.x, ctx->input.y);
    bool mouse_on_thumb = rect_contains(thumb_rect, ctx->input.x, ctx->input.y);

    if (mouse_on_track || mouse_on_thumb) {
        state |= UI_STATE_HOVERED;
    }
    if (ctx->input.left_down && (mouse_on_track || mouse_on_thumb || scroll_state->drag_active_h)) {
        state |= UI_STATE_PRESSED;
    }
    if (ctx->input.left_clicked && (mouse_on_track || mouse_on_thumb)) {
        state |= UI_STATE_CLICKED;
    }

    // Drag start: mouse pressed on the thumb this frame
    if (ctx->input.left_down && mouse_on_thumb && !scroll_state->drag_active_h
        && !ctx->input.left_clicked) {
        scroll_state->drag_active_h = true;
        scroll_state->drag_start_mouse = ctx->input.x;
        scroll_state->drag_start_offset = scroll_state->offset_x;
    }

    // Continue drag
    if (scroll_state->drag_active_h) {
        if (ctx->input.left_down) {
            float delta_px = (float)(ctx->input.x - scroll_state->drag_start_mouse);
            float track_usable = viewport_w - thumb_w;
            if (track_usable > 0.0f && max_offset > 0.0f) {
                float new_offset = scroll_state->drag_start_offset
                    + (delta_px * (max_offset / track_usable));
                scroll_state->offset_x = medit_clampf(new_offset, 0.0f, max_offset);
            }
        } else {
            scroll_state->drag_active_h = false;
        }
    } else if ((state & UI_STATE_PRESSED) && !mouse_on_thumb) {
        // Click on track outside thumb - jump
        float rel = ((float)ctx->input.x - (float)track.x - (thumb_w / 2.f))
            / (viewport_w - thumb_w);
        rel = medit_clampf(rel, 0.0f, 1.0f);
        if (max_offset > 0.0f) {
            scroll_state->offset_x = rel * max_offset;
        }
    }

    // Recompute for draw command after potential offset change
    scrollbar_geometry(
        scroll_state->offset_x,
        content_w,
        viewport_w,
        viewport_w,
        &scroll_pos,
        &thumb_ratio);

    UiDrawCmd scrollbar = {
        .kind = UI_CMD_SCROLLBAR,
        .rect = track,
        .color = track_color,
        .thumb_color = thumb_color,
        .scroll_pos = scroll_pos,
        .thumb_ratio = thumb_ratio,
        .is_horizontal = true,
    };
    draw_cmd_list_push(ctx->draw_list, scrollbar);

    return state;
}

Rect ui_scroll_begin(UiCtx* ctx, Rect viewport, UiScrollState* scroll_state)
{
    UiDrawCmd clip = {
        .kind = UI_CMD_CLIP_PUSH,
        .rect = viewport,
    };
    draw_cmd_list_push(ctx->draw_list, clip);

    // With int32_t coordinates, subtraction is naturally signed — no saturation needed.
    int32_t ox = float_to_i32(medit_clampf(scroll_state->offset_x, 0.0f, scroll_state->content_w));
    int32_t oy = float_to_i32(medit_clampf(scroll_state->offset_y, 0.0f, scroll_state->content_h));

    Rect content = viewport;
    content.x = viewport.x - ox;
    content.y = viewport.y - oy;
    content.w = (scroll_state->content_w > 0.0f) ? float_to_i32(scroll_state->content_w)
                                                 : viewport.w;
    content.h = (scroll_state->content_h > 0.0f) ? float_to_i32(scroll_state->content_h)
                                                 : viewport.h;

    return content;
}

void ui_scroll_end(UiCtx* ctx, Rect viewport, UiScrollState* scroll_state, bool hovered)
{
    UiDrawCmd clip = {
        .kind = UI_CMD_CLIP_POP,
        .rect = viewport,
    };
    draw_cmd_list_push(ctx->draw_list, clip);

    if (!hovered || !ctx->input.scroll_valid) {
        return;
    }

    float max_y = scroll_state->content_h - (float)viewport.h;
    float max_x = scroll_state->content_w - (float)viewport.w;

    if (max_y > 0.0f) {
        scroll_state->offset_y = medit_clampf(
            scroll_state->offset_y - (ctx->input.scroll_y * ctx->scroll_speed),
            0.0f,
            max_y);
    }
    if (max_x > 0.0f) {
        scroll_state->offset_x = medit_clampf(
            scroll_state->offset_x - (ctx->input.scroll_x * ctx->scroll_speed),
            0.0f,
            max_x);
    }
}
