#ifndef MEDIT_CORE_UI_H_
#define MEDIT_CORE_UI_H_

#include "color.h"
#include "rect.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t x, y; // current mouse position on screen (UI-dependent, could be pixels or cells)
    float scroll_x, scroll_y; // scroll wheel delta
    bool scroll_valid;
    bool left_down; // is left button currently held
    bool left_clicked; // was left button released this frame
} UiInputEvent;

#define UI_STATE_HOVERED (1ul << 0ul)
#define UI_STATE_PRESSED (1ul << 1ul)
#define UI_STATE_CLICKED (1ul << 2ul)
typedef unsigned long UiWidgetState;

typedef enum {
    UI_CMD_RECT_FILLED,
    UI_CMD_RECT_OUTLINED,
    UI_CMD_TEXT,
    UI_CMD_CLIP_PUSH,
    UI_CMD_CLIP_POP,
    UI_CMD_SCROLLBAR,
} UiCmdKind;

typedef struct {
    UiCmdKind kind;
    // common data
    Rect rect;
    Color color;
    // command-specific data
    union {
        // specific to UI_CMD_RECT_OUTLINED
        Color outline_color;
        // specific to UI_CMD_TEXT
        const char* text;
        // specific to UI_CMD_SCROLLBAR
        struct {
            Color thumb_color;
            float scroll_pos; // 0..1 normalized thumb position
            float thumb_ratio; // thumb_size / track_size, clamped 0..1
            bool is_horizontal; // true → horizontal scrollbar, false → vertical
        };
    };
} UiDrawCmd;

typedef struct {
    UiDrawCmd* items;
    size_t count;
    size_t capacity;
} UiDrawCmdList;

void ui_draw_cmd_list_init(UiDrawCmdList* draw_cmd_list);
void ui_draw_cmd_list_free(UiDrawCmdList* draw_cmd_list);
void ui_draw_cmd_list_clear(UiDrawCmdList* draw_cmd_list);

// Scrollable area state (caller-owned, persists across frames)
typedef struct {
    float offset_x;
    float offset_y;
    float content_w; // total content width  (set by caller)
    float content_h; // total content height (set by caller)

    // Thumb drag state - managed by ui_scrollbar_v / ui_scrollbar_h
    int32_t drag_start_mouse; // mouse coord at drag start (y or x)
    float drag_start_offset; // offset_y or offset_x at drag start
    bool drag_active_v; // vertical thumb is being dragged
    bool drag_active_h; // horizontal thumb is being dragged
} UiScrollState;

// Context (one per window, reset each frame)
typedef struct {
    UiInputEvent input;
    UiDrawCmdList* draw_list;
    // Pixels for GUI or rows/cells for terminal backends scrolled per wheel notch
    float scroll_speed;
} UiCtx;

// Simple filled panel - emits UI_CMD_RECT_FILLED
void ui_panel(UiCtx* ctx, Rect rect, Color bg);

// Outlined panel - emits UI_CMD_RECT_FILLED + UI_CMD_RECT_OUTLINED
void ui_panel_outlined(UiCtx* ctx, Rect rect, Color bg, Color outline);

// Button - emits a filled rect and a label, colors change on hover/press
// Returns a bitmask of UiWidgetState flags
UiWidgetState ui_button(
    UiCtx* ctx,
    Rect rect,
    Color bg,
    Color hover_bg,
    Color press_bg,
    const char* label,
    Rect* label_rect_opt,
    Color label_color);

// Scrolling API general usage (when z-order doesn't matter, e.g. scrollbar beside content):
//
//   scroll_state.content_w = total_content_width;
//   scroll_state.content_h = total_content_height;
//   ui_scrollbar_v(&ctx, scrollbar_rect, &scroll_state, track_color, thumb_color);
//   Rect content = ui_scroll_begin(&ctx, viewport, &scroll_state);
//   ... draw child widgets using content.x / content.y as origin ...
//   ui_scroll_end(&ctx, viewport, &scroll_state, hovered);
//
// The scrollbar is placed outside the viewport so it never overlaps content; z-order is fine even
// though input and draw happen before ui_scroll_begin.
//
// NOTE about scroll widget z-ordering:
//
// Scrollbars can be drawn on top of (i.e. overlapping) the scrollable content - so the scrollbar
// draw call must come AFTER the content elements in the draw sequence. However, the scrollbar also
// processes user input to update the scroll offset, and the scrollable content must use that
// updated offset to render at the correct position. This creates a conflict:
// - if you call ui_scrollbar_{v,h} (input + draw) BEFORE the content, the thumb renders under the
//   content (wrong z-order) but the offset is fresh
// - if you call ui_scrollbar_{v,h} AFTER the content, the thumb overlays correctly but the content
//   was rendered with the previous frame's offset (one-frame lag)
//
// To resolve this, use the split API:
// - ui_scrollbar_{v,h}_update  <- call BEFORE ui_scroll_begin (processes input, no draw)
// - ui_scroll_begin
// - ... draw child widgets ...
// - ui_scroll_end
// - ui_scrollbar_{v,h}_draw    <- call AFTER ui_scroll_end (emits draw command, no input)
//
// This way the content and the thumb both use the same up-to-date offset within the same frame,
// and the thumb is drawn on top.

// Begin a scrollable region
// Pushes a clip rect matching viewport, then returns the inner content rect whose origin is offset
// by scroll_state so child widgets lay out in content-space coordinates.
// With int32_t coordinates, rect.x/y can be negative - no saturation needed.
Rect ui_scroll_begin(UiCtx* ctx, Rect viewport, UiScrollState* scroll_state);

// End a scrollable region
// Pops the clip rect and applies wheel delta to scroll_state when hovered
// NOTE: scroll_state->content_w / content_h must be set by the caller before this call so clamping
//       works correctly
void ui_scroll_end(UiCtx* ctx, Rect viewport, UiScrollState* scroll_state, bool hovered);

// Vertical scrollbar - input phase only (no draw command emitted).
// Updates scroll_state->offset_y from drag and click-on-track interactions.
// Call this BEFORE ui_scroll_begin so the scrollable content renders with the fresh offset.
// Returns a bitmask of UiWidgetState flags.
UiWidgetState ui_scrollbar_v_update(UiCtx* ctx, Rect track, UiScrollState* scroll_state);

// Vertical scrollbar - draw phase only (no input processing).
// Emits UI_CMD_SCROLLBAR using the current offset_y.
// Call this AFTER drawing the scrollable content so the thumb overlays it correctly.
void ui_scrollbar_v_draw(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color);

// Vertical scrollbar - combined convenience wrapper (input + draw in one call).
// Handles click-on-track to jump and drag-to-scroll on the thumb.
// Updates scroll_state->offset_y on interaction.
// Returns a bitmask of UiWidgetState flags.
UiWidgetState ui_scrollbar_v(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color);

// Horizontal scrollbar - input phase only (no draw command emitted)
// Updates scroll_state->offset_x from drag and click-on-track interactions
// Call this BEFORE ui_scroll_begin so the scrollable content renders with the fresh offset.
// Returns a bitmask of UiWidgetState flags.
UiWidgetState ui_scrollbar_h_update(UiCtx* ctx, Rect track, UiScrollState* scroll_state);

// Horizontal scrollbar - draw phase only (no input processing).
// Emits UI_CMD_SCROLLBAR using the current offset_x.
// Call this AFTER drawing the scrollable content so the thumb overlays it correctly.
void ui_scrollbar_h_draw(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color);

// Horizontal scrollbar - combined convenience wrapper (input + draw in one call).
// Use when z-ordering relative to the scrollable content doesn't matter.
UiWidgetState ui_scrollbar_h(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color);

#endif // MEDIT_CORE_UI_H_
