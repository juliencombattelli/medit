#ifndef UI22_H_
#define UI22_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

///=============================================================================
/// General data structures
///=============================================================================

typedef struct {
    int32_t x, y;
} Ui2Position;

typedef struct {
    int32_t width, height;
} Ui2Dimension;

typedef struct {
    Ui2Position pos;
    Ui2Dimension dim;
} Ui2BoundingBox;

typedef struct {
    uint8_t r, g, b, a;
} Ui2Color;

///=============================================================================
/// Draw commands
///=============================================================================

typedef enum {
    UI2_DRAW_CMD_RECT_FILLED,
    UI2_DRAW_CMD_TEXT,
    UI2_DRAW_CMD_CLIP_PUSH,
    UI2_DRAW_CMD_CLIP_POP,
} Ui2DrawCmdKind;

typedef struct {
    Ui2Color background_color;
} Ui2DrawCommandDataRect;

typedef struct {
    const char* text;
    size_t length;
    Ui2Color color;
    // TODO add font settings like a font_id, font_size, letter_spacing, line_height
} Ui2DrawCommandDataText;

typedef struct {
    Ui2DrawCmdKind kind;
    Ui2BoundingBox bounding_box;
    union {
        Ui2DrawCommandDataRect rect;
        Ui2DrawCommandDataText text;
    };
} Ui2DrawCmd;

typedef struct {
    Ui2DrawCmd* items;
    size_t count;
    size_t capacity;
} Ui2DrawCmdList;

///=============================================================================
/// Context definition
///=============================================================================

typedef struct {
    char* memory;
    size_t capacity;
} Ui2Arena;

typedef struct {
    Ui2Arena arena;
    Ui2DrawCmdList draw_cmd_list;
} Ui2Context;

///=============================================================================
/// Ui2 elements definition
///=============================================================================

typedef enum {
    UI2_SIZING_TYPE_FIT,
    UI2_SIZING_TYPE_GROW,
    UI2_SIZING_TYPE_PERCENT,
    UI2_SIZING_TYPE_FIXED,
} Ui2SizingType;

typedef enum {
    UI2_ALIGN_X_LEFT,
    UI2_ALIGN_X_RIGHT,
    UI2_ALIGN_X_CENTER,
} Ui2LayoutAlignmentX;

typedef enum {
    UI2_ALIGN_Y_TOP,
    UI2_ALIGN_Y_BOTTOM,
    UI2_ALIGN_Y_CENTER,
} Ui2LayoutAlignmentY;

typedef enum {
    UI2_LEFT_TO_RIGHT,
    UI2_TOP_TO_BOTTOM,
} Ui2LayoutDirection;

typedef struct {
    int32_t min, max;
} Ui2SizingMinMax;

typedef struct {
    Ui2SizingType type;
    union {
        Ui2SizingMinMax minMax;
        float percent;
    } size;
} Ui2SizingAxis;

typedef struct {
    Ui2SizingAxis width;
    Ui2SizingAxis height;
} Ui2Sizing;

typedef struct {
    uint16_t left;
    uint16_t right;
    uint16_t top;
    uint16_t bottom;
} Ui2Padding;

typedef struct Clay_ChildAlignment {
    Ui2LayoutAlignmentX x;
    Ui2LayoutAlignmentY y;
} Ui2ChildAlignment;

typedef struct {
    Ui2Sizing sizing;
    Ui2Padding passing;
    uint16_t child_gap;
    Ui2ChildAlignment child_alignment;
} Ui2Layout;

typedef struct {
    Ui2Layout layout;
    Ui2Color background_color;
} Ui2ElementDescriptor;

///=============================================================================
/// Ui22 functions definition
///=============================================================================

void ui2_init(Ui2Context* ctx, Ui2Arena arena);

void ui2_frame_begin(Ui2Context* ctx);
void ui2_element_begin(Ui2Context* ctx, Ui2ElementDescriptor desc);
void ui2_element_end(Ui2Context* ctx);
Ui2DrawCmdList ui2_frame_end(Ui2Context* ctx);

#endif // UI22_H_
