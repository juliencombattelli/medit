#ifndef MEDIT_CORE_UI_H_
#define MEDIT_CORE_UI_H_

#include "color.h"
#include "rect.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t x, y; // current mouse position on screen (UI-dependent, could be pixels or cells)
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
        struct {
            const char* text;
            ptrdiff_t text_x; // signed screen-space x for the label (may be negative when scrolled)
            ptrdiff_t text_y; // signed screen-space y for the label
        };
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
    size_t drag_start_mouse; // mouse coord at drag start (y or x)
    float drag_start_offset; // offset_y or offset_x at drag start
    bool drag_active_v; // vertical thumb is being dragged
    bool drag_active_h; // horizontal thumb is being dragged
} UiScrollState;

// Return value of ui_scroll_begin.
// rect  — safe content rect (x/y saturated to 0 so size_t never wraps); suitable for
//         panel_cut_left / panel_cut_top when the scroll offset ≤ viewport.x/y.
// origin_x / origin_y — true signed content origin in screen space; use these for layout
//         whenever the scroll offset can exceed viewport.x/y (e.g. a tab bar whose
//         viewport starts near x = 0 and may scroll many pixels to the right).
typedef struct {
    Rect rect;
    ptrdiff_t origin_x;
    ptrdiff_t origin_y;
} UiScrollContent;

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
    Color label_color);

// Vertical scrollbar - emits UI_CMD_SCROLLBAR
// Handles click-on-track to jump and drag-to-scroll on the thumb
// Updates scroll_state->offset_y on interaction
// Returns a bitmask UiWidgetState flags
// NOTE: pass left_clicked=true for the single frame the mouse button is released so drag state is
//       cleared correctly
UiWidgetState ui_scrollbar_v(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color);

// Horizontal scrollbar - mirror of ui_scrollbar_v for offset_x
UiWidgetState ui_scrollbar_h(
    UiCtx* ctx,
    Rect track,
    UiScrollState* scroll_state,
    Color track_color,
    Color thumb_color);

// Begin a scrollable region.
// Pushes a clip rect matching viewport.
// Returns a UiScrollContent whose:
//   .rect     has x/y saturated to 0 so size_t never wraps — use with panel_cut_left/top when
//             the scroll offset is guaranteed to be ≤ viewport.x/y.
//   .origin_x / .origin_y are the true signed content origins — use these for manual layout
//             when the scroll offset may exceed the viewport's screen-space position.
UiScrollContent ui_scroll_begin(UiCtx* ctx, Rect viewport, UiScrollState* scroll_state);

// End a scrollable region
// Pops the clip rect and applies wheel delta to scroll_state when hovered
// NOTE: scroll_state->content_w / content_h must be set by the caller before this call so clamping
//       works correctly
void ui_scroll_end(UiCtx* ctx, Rect viewport, UiScrollState* scroll_state, bool hovered);

#endif // MEDIT_CORE_UI_H_
