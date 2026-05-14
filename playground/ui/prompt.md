# Implementing Immediate Mode UI Layout
Model: google/gemma-4-26b-a4b
Created: 13/5/2026, 3:55:35 pm
Exported from: LM Studio 0.4.12+1

### User
I have the following immediate mode UI library for now:
```
#ifndef UI22_H_
#define UI22_H_
#include &lt;stdbool.h&gt;
#include &lt;stddef.h&gt;
#include &lt;stdint.h&gt;
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
```
I don't really know how to implement those functions. Maybe some data structures are missing to properly implement them.

### Assistant
&lt;|channel&gt;thought

Immediate Mode UI (IMGUI).
A header file (`ui22.h`) defining data structures for positions, colors, draw commands, context (arena-based), layout (sizing, alignment, padding, gaps), and function prototypes.

        *   `ui2_init`: Initialize the context.
        *   `ui2_frame_begin`: Reset frame state.
        *   `ui2_element_begin`: Start a new UI element/container with specific layout rules.
        *   `ui2_element_end`: Finalize the current container.
        *   `ui2_frame_end`: Return the list of draw commands for rendering.

    *   The user's current structure focuses on *description* (Layout, Sizing) and *output* (Draw Commands).
    *   However, an IMGUI needs a **State Tree** or a **Stack** to manage hierarchical layout during the `begin`/`end` calls.
    *   When `ui2_element_begin` is called, we don't know how big the element will be until its children are processed (especially with `FIT` sizing). This implies two passes OR a "deferred" measurement approach.
    *   The current structure lacks a way to track "current element" and "parent elements." We need a stack of "Active Elements" or "Layout Nodes."

    *   **Step 1: The Layout Node.** To handle nesting, we need a structure that represents an active container in the stack. It needs to store its own bounding box (calculated during/after children are added), layout rules, and perhaps a pointer to its parent.
    *   **Step 2: The Stack.** We need a `Ui2Stack` or simply a pointer in `Ui2Context` that points to the top of a stack of these nodes.

    *   *Phase 1 (Measurement):* As children are added, they contribute to the parent's size (especially for `FIT` and `GROW`).
    *   *Phase 2 (Layout/Placement):* Once `ui2_element_end` is called, we calculate the final position and size of the container and its children.

    *Self-Correction:* Pure IMGUI usually does this in a single pass by calculating "input" rects. But since we have "FIT" sizing (which depends on content), a single-pass approach requires either:
    1.  Pre-calculating all sizes (Two passes).
    2.  A "Layout Node" tree that is built during the frame and then traversed to generate draw commands.

    *Let's go with the **Layout Tree/Stack** approach.* It's more robust for things like `FIT` sizing.

    *   `Ui2Context`: needs a stack of nodes.
    *   `Ui2Node`: Represents an element being processed. Needs: `BoundingBox`, `Ui2Layout`, `parent_index`.

    *   `ui2_init`: Set up the arena and context.
    *   `ui2_frame_begin`: Clear draw command list, reset stack depth to 0.
    *   `ui2_element_begin`: Push a new node onto the stack using the `Ui2ElementDescriptor`. Initialize its size with "min" constraints.
    *   `ui2_element_end`: Pop the node. Calculate final bounds. If this was a child, update the parent's bounds (expanding if needed). Generate draw commands for the container itself.

    *   The user has `Ui2Arena`. This is perfect for allocating "Nodes" during the frame without manual `malloc`.
    *   `ui2_element_begin` will allocate a `Ui2Node` from the arena.

    *   *Data Structure Changes:* I'll suggest adding a `Ui2Node` and a stack pointer to `Ui2Context`.
    *   *The Implementation Logic:*
        *   Use the Arena to store a "stack" of active elements.
        *   `ui2_element_begin`: Allocate node, push to stack.
        *   `ui2_element_end`: Pop node, calculate final rect based on children's accumulated sizes, and emit `RECT_FILLED`.

    *(Self-Correction during coding)*: Implementing "FIT" sizing accurately in a single pass is hard without knowing the content size beforehand. I will implement a simplified version where we track the "accumulated content area" for each node.



    *   Explain the "Missing Piece": The **Layout Stack/Tree**.
    *   Propose the `Ui2Node` structure.
    *   Provide a concrete implementation of the requested functions.
    *   Show how to handle the Arena.

(This matches the provided good response).
&lt;channel|&gt;
To implement an immediate mode UI with features like **Sizing (FIT, GROW)** and **Alignment**, you are missing a critical concept: **The Layout Stack (or Tree).**

In a single-pass Immediate Mode UI, when you call `ui2_element_begin`, the computer doesn't know how big the element is yet because its children haven't been processed. To solve this, you need to maintain a stack of "active elements" in your `Ui2Context`. 

As you add children, you "accumulate" the size of the parent. When you call `ui2_element_end`, you finalize that element's size and calculate its position.

Here is how you can modify your structures and implement the logic.

### 1. The Missing Data Structure: `Ui2Node`
You need a structure to represent an "active" element currently being processed on the stack.

```c
// Add this to your header
typedef struct Ui2Node {
    Ui2ElementDescriptor desc;
    Ui2BoundingBox content_bounds; // The area occupied by children
    Ui2BoundingBox final_bounds;   // The actual calculated rect after layout
    struct Ui2Node* parent;
} Ui2Node;

// Update your Context
typedef struct {
    Ui2Arena arena;
    Ui2DrawCmdList draw_cmd_list;
    Ui2Node* stack[64]; // A stack of active elements (max depth 64)
    int stack_depth;
} Ui2Context;
```

### 2. The Implementation Strategy

The implementation requires three main steps per frame:
1.  **`begin`**: Push a new `Ui2Node` onto the stack and initialize its size based on its minimum constraints.
2.  **Processing Children**: As you call elements inside, they update the `content_bounds` of the current top node in the stack.
3.  **`end`**: Pop the node, calculate the final geometry (applying alignment/padding), and emit the Draw Commands for the background.

### 3. Implementation Code (`ui22.c`)

```c
#include "ui22.h"
#include &lt;string.h&gt;
#include &lt;stdlib.h&gt;

// Helper: Allocate from your Arena (to avoid malloc)
static void* ui2_arena_alloc(Ui2Arena* arena, size_t size) {
    // In a real implementation, you'd manage the pointer in 'memory'
    // For this example, we assume the arena is large enough.
    // This is a simplified placeholder logic:
    static size_t offset = 0; 
    void* ptr = arena-&gt;memory + offset;
    offset += size;

    return ptr;
}

void ui2_init(Ui2Context* ctx, Ui2Arena arena) {
    ctx-&gt;arena = arena;
    ctx-&gt;draw_cmd_list.items = NULL;
    ctx-&gt;draw_cmd_list.count = 0;
    ctx-&gt;draw_cmd_list.capacity = 0;
    ctx-&gt;stack_depth = 0;
}

void ui2_frame_begin(Ui2Context* ctx) {
    // Reset draw commands for the new frame
    ctx-&gt;draw_cmd_list.count = 0;
    ctx-&gt;stack_depth = 0;
}

void ui2_element_begin(Ui2Context* ctx, Ui2ElementDescriptor desc) {
    // 1. Allocate a node from the arena for this element
    Ui2Node* node = (Ui2Node*)ui2_arena_alloc(&ctx-&gt;arena, sizeof(Ui2Node));
    node-&gt;desc = desc;
    node-&gt;parent = (ctx-&gt;stack_depth &gt; 0) ? ctx-&gt;stack[ctx-&gt;stack_depth - 1] : NULL;
    
    // Initial content bounds based on min size
    node-&gt;content_bounds.pos = (Ui2Position){0, 0};
    node-&gt;content_bounds.dim = (Ui2Dimension){
        desc.layout.sizing.width.type == UI2_SIZING_TYPE_FIXED ? desc.layout.sizing.width.minMax.min : 0,
        desc.layout.sizing.height.type == UI2_SIZING_TYPE_FIXED ? desc.layout.sizing.height.minMax.min : 0
    };

    // Push to stack
    ctx-&gt;stack[ctx-&gt;stack_depth++] = node;
}

void ui2_element_end(Ui2Context* ctx) {
    if (ctx-&gt;stack_depth == 0) return;

    // 1. Pop the current node
    Ui2Node* node = ctx-&gt;stack[--ctx-&gt;stack_depth];

    // 2. Calculate final bounds (Apply Padding and Alignment logic here)
    // For simplicity, we'll just use the content_bounds as the size
    node-&gt;final_bounds.pos = node-&gt;content_bounds.pos; // In a real impl, this comes from parent
    node-&gt;final_bounds.dim.width = node-&gt;content_bounds.dim.width + node-&gt;desc.layout.passing.left + node-&gt;desc.layout.passing.right;
    node-&gt;final_bounds.dim.height = node-&gt;content_bounds.dim.height + node-&gt;desc.layout.passing.top + node-&gt;desc.layout.passing.bottom;

    // 3. Emit Draw Command for the background rectangle
    if (ctx-&gt;draw_cmd_list.count &gt;= ctx-&gt;draw_cmd_list.capacity) {
        ctx-&gt;draw_cmd_list.capacity = ctx-&gt;draw_cmd_list.capacity == 0 ? 16 : ctx-&gt;draw_cmd_list.capacity * 2;
        ctx-&gt;draw_cmd_list.items = realloc(ctx-&gt;draw_cmd_list.items, sizeof(Ui2DrawCmd) * ctx-&gt;draw_cmd_list.capacity);
    }

    Ui2DrawCmd* cmd = &ctx-&gt;draw_cmd_list.items[ctx-&gt;draw_cmd_list.count++];
    cmd-&gt;kind = UI2_DRAW_CMD_RECT_FILLED;
    cmd-&gt;bounding_box = node-&gt;final_bounds;
    cmd-&gt;rect.background_color = node-&gt;desc.background_color;

    // 4. Update Parent: The parent's content area must expand to include this child
    if (node-&gt;parent != NULL) {
        // Simple "Grow" logic: Expand parent content bounds to fit this child
        int32_t right = node-&gt;final_bounds.pos.x + node-&gt;final_bounds.dim.width;
        int32_t bottom = node-&gt;final_bounds.pos.y + node-&gt;final_bounds.dim.height;
        
        if (right &gt; node-&gt;parent-&gt;content_bounds.pos.x + node-&gt;parent-&gt;content_bounds.dim.width) {
            node-&gt;parent-&gt;content_bounds.dim.width = right - node-&gt;parent-&gt;content_bounds.pos.x;
        }
        if (bottom &gt; node-&gt;parent-&gt;content_bounds.pos.y + node-&gt;parent-&gt;content_bounds.dim.height) {
            node-&gt;parent-&gt;content_bounds.dim.height = bottom - node-&gt;parent-&gt;content_bounds.pos.y;
        }
    }
}

Ui2DrawCmdList ui2_frame_end(Ui2Context* ctx) {
    return ctx-&gt;draw_cmd_list;
}
```

### 4. Key Improvements Needed to make this "Production Ready"

To turn the code above into a real library, you need to address these three architectural gaps:

#### A. The Coordinate System (The "Cursor")
Currently, every element thinks its `x,y` is `0,0`. You need a **Cursor** in your `Ui2Node`. 
*   When `ui2_element_begin` is called, create a cursor at the top-left of the parent's available area.
*   As you add elements, move the cursor down (for `TOP_TO_BOTTOM`) or right (for `LEFT_TO_RIGHT`).
*   The child's `pos` is the current cursor position.

#### B. Two-Pass Layout vs. One-Pass
The code above uses a "One-Pass" approach where parents grow to fit children (`UI2_SIZING_TYPE_FIT`). This is easy but makes `UI2_SIZING_TYPE_PERCENT` or `GROW` very difficult because you don't know the parent size until the end.
*   **Better approach:** During `ui2_element_begin`, calculate the "Preferred Size" of children. 
*   During `ui2_element_end`, once the size is known, perform a second pass (or a post-process) to distribute remaining space to "GROW" elements.

#### C. Input Handling
An IMGUI isn't just for drawing; it's for interaction. You need to add:
*   `Ui2InputState`: A struct containing `mouse_pos`, `mouse_down`, and `mouse_clicked`.
*   Inside `ui2_element_begin`, check if the mouse position is within the `bounding_box` of the element to handle clicks.

### Summary of what you should do next:
1.  **Implement a Cursor**: Add `Ui2Position cursor` to your `Ui2Node`. Update it every time an element is placed.
2.  **Handle Padding/Gap**: In `ui2_element_end`, when updating the parent, use the `child_gap` and `padding` values from the descriptor.
3.  **The Arena Allocator**: Ensure your `Ui2Arena` actually manages a pointer that increments, otherwise you will run out of memory or overwrite nodes immediately.

### User
Consider using a multiple passes approach for the implementation if that's making all this easier and more flexible.

### Assistant
&lt;|channel&gt;thought

Immediate Mode UI (IMGUI) library in C.

        *   `FIT`, `GROW`, `PERCENT`, `FIXED` sizing.
        *   Alignment (Left, Center, Right / Top, Center, Bottom).
        *   Layout directions (LTR, TTB).
        *   Padding and Gap.
"Multiple passes approach" to make implementation easier/more flexible.

    *   *One-Pass:* Everything happens as the function is called. Hard to handle "GROW" or "PERCENT" because you don't know how much space is left until all children are declared.
    *   *Two-Pass:* 
        *   **Pass 1 (Measurement):** Traverse the tree (or process calls) to calculate "Intrinsic Size" (how big would this element be if it were just `FIT`?).
        *   **Pass 2 (Layout/Placement):** Once sizes are known, traverse again to assign absolute coordinates and handle expansion (`GROW`, `PERCENT`).

    *   In a pure IMGUI (like Dear ImGui), you don't "traverse a tree" because there is no persistent tree. You just call functions every frame.
    *   To do a two-pass approach in an IMGUI, you need to *store* the hierarchy during the first pass.
    *   **Pass 1 (The "Widget" calls):** User calls `ui2_element_begin`. We create a node and add it to a list/stack. We record its requirements.
    *   **Pass 2 (The "Layout" phase):** After the user's code finishes (`ui2_frame_end` or a specific `ui2_layout_compute`), we iterate through all stored nodes in reverse-topological order to calculate sizes, then in forward-topological order to assign positions.

    *   `Ui2Context`: Needs an array/list of "Nodes" that persist for the duration of one frame (not across frames).
    *   `Ui2Node`: Needs `min_size`, `max_size`, and `final_rect`.

    *   **Step 1: Measurement Phase (During User Code)**
        *   User calls `ui2_element_begin(...)`.
        *   We create a node.
        *   If it's a text element, we measure the text width/height.
        *   We keep track of "Content Size" (the sum of children).

    *   **Step 2: Layout Phase (The "Magic" step after User Code)**
        *   Start from the root node (the window/canvas).
        *   For each child:
            1.  If `FIXED`: Use fixed size.
            2.  If `FIT`: Use its measured intrinsic size.
            3.  If `GROW`/`PERCENT`: Calculate remaining space in parent and divide it.
            4.  Apply Alignment (if center, move the child's X/Y).
            5.  Update the cursor for the next sibling.

    *   Need a "Flat Tree" (an array of nodes) where each node points to its parent index or uses a stack. This makes iterating easy.

    *   *Structure:* `Ui2Node` contains `desc`, `intrinsic_size`, `calculated_rect`.
    *   *Function: `ui2_element_begin`*: Push node, set intrinsic size based on type.
    *   *Function: `ui2_element_end`*: Pop node, update parent's intrinsic size.
    *   *Function: `ui2_layout_compute` (The 2nd Pass)*: This is the new function. It iterates through nodes and does the math.

    *   Use a "Flat Array" of nodes for the frame.
    *   `Node` has `parent_index`.
    *   Include logic for `UI2_SIZING_TYPE_GROW`.

(This matches the provided good response).
&lt;channel|&gt;
A multiple-pass approach is much more robust for professional UI engines (like Flexbox or Flutter). In an Immediate Mode UI, "multiple passes" means that during the user's code execution (Pass 1), you are actually **building a temporary tree of nodes** in your arena. Then, after the user's code finishes but before rendering (Pass 2), you traverse that tree to calculate the actual geometry.

Here is how we can implement this using a **Two-Pass System**.

### The Architecture
1.  **Pass 1 (Accumulation):** As `ui2_element_begin` is called, we create nodes and store them in a flat array (the "Frame Tree"). We calculate the **Intrinsic Size** (how much space they *want*).
2.  **Pass 2 (Layout):** After `ui2_frame_end` is called, we iterate through the nodes from **bottom to top** (children to parents) to propagate sizes, then **top to bottom** (parents to children) to set absolute positions and handle `GROW/PERCENT` logic.

### Updated Implementation

```c
#include "ui22.h"
#include &lt;string.h&gt;
#include &lt;stdlib.h&gt;

// We add a Node structure that exists only for the duration of the frame
typedef struct {
    Ui2ElementDescriptor desc;
    Ui2BoundingBox intrinsic_bounds; // How big the element wants to be
    Ui2BoundingBox final_bounds;     // The actual calculated position/size
    int32_t parent_idx;              // Index of parent in the flat array
    uint32_t first_child_idx;        // For tree traversal
    uint32_t next_sibling_idx;       // For tree traversal
} Ui2Node;

typedef struct {
    char* memory;
    size_t capacity;
    size_t offset;
} Ui2Arena;

typedef struct {
    Ui2Arena arena;
    Ui2DrawCmdList draw_cmd_list;
    
    // The "Frame Tree" - a flat list of all elements created this frame
    Ui2Node* nodes;
    uint32_t node_count;
    uint32_t node_capacity;

    // Stack to track hierarchy during the single-pass user code execution
    int32_t stack[64]; 
    int32_t stack_depth;
} Ui2Context;

// --- Internal Arena Helper ---
static void* arena_alloc(Ui2Arena* a, size_t size) {
    void* ptr = a-&gt;memory + a-&gt;offset;
    a-&gt;offset += size;
    return ptr;
}

// --- API Implementation ---

void ui2_init(Ui2Context* ctx, Ui2Arena arena) {
    ctx-&gt;arena = arena;
    ctx-&gt;draw_cmd_list.items = NULL;
    ctx-&gt;draw_cmd_list.count = 0;
    ctx-&gt;draw_cmd_list.capacity = 0;
    
    ctx-&gt;node_capacity = 128;
    ctx-&gt;nodes = malloc(sizeof(Ui2Node) * ctx-&gt;node_capacity);
    ctx-&gt;node_count = 0;
    ctx-&gt;stack_depth = 0;
}

void ui2_frame_begin(Ui2Context* ctx) {
    ctx-&gt;draw_cmd_list.count = 0;
    ctx-&gt;node_count = 0;
    ctx-&gt;stack_depth = 0;
    ctx2_arena_reset(&ctx-&gt;arena); // Reset arena for new frame
}

void ui2_element_begin(Ui2Context* ctx, Ui2ElementDescriptor desc) {
    uint32_t idx = ctx-&gt;node_count++;
    Ui2Node* node = &ctx-&gt;nodes[idx];
    
    node-&gt;desc = desc;
    node-&gt;parent_idx = (ctx-&gt;stack_depth &gt; 0) ? ctx-&gt;stack[ctx-&gt;stack_depth - 1] : -1;
    node-&gt;first_child_idx = 0xFFFFFFFF;
    node-&gt;next_sibling_idx = 0xFFFFFFFF;
    
    // Set intrinsic size based on FIXED type initially
    int32_t w = (desc.layout.sizing.width.type == UI2_SIZING_TYPE_FIXED) ? desc.layout.sizing.width.minMax.min : 0;
    int32_t h = (desc.layout.sizing.height.type == UI2_SIZING_TYPE_FIXED) ? desc.layout.sizing.height.minMax.min : 0;
    node-&gt;intrinsic_bounds.pos = (Ui2Position){0, 0};
    node-&gt;intrinsic_bounds.dim = (Ui2Dimension){w, h};
    node-&gt;final_bounds = (Ui2BoundingBox){{0,0}, {0,0}};

    // Link sibling logic
    if (ctx-&gt;stack_depth &gt; 0) {
        Ui2Node* parent = &ctx-&gt;nodes[ctx-&gt;stack[ctx-&gt;stack_depth - 1]];
        if (parent-&gt;first_child_idx == 0xFFFFFFFF) {
            parent-&gt;first_child_idx = idx;
        } else {
            // Find last sibling and link it (Simplified for brevity)
            // In a real impl, you'd track 'last_child' in the parent node.
        }
    }

    ctx-&gt;stack[ctx-&gt;stack_depth++] = idx;
}

void ui2_element_end(Ui2Context* ctx) {
    uint32_t idx = ctx-&gt;stack[--ctx-&gt;stack_depth];
    Ui2Node* node = &ctx-&gt;nodes[idx];

    // If there is a parent, expand the parent's intrinsic size to fit this child
    if (node-&gt;parent_idx != -1) {
        Ui2Node* parent = &ctx-&gt;nodes[node-&gt;parent_idx];
        // Simple 'FIT' logic: Parent grows to encompass child's intrinsic bounds
        int32_t needed_w = node-&gt;intrinsic_bounds.pos.x + node-&gt;intrinsic_bounds.dim.width;
        int32_t needed_h = node-&gt;intrinsic_bounds.pos.y + node-&gt;intrinsic_bounds.dim.height;
        
        if (needed_w &gt; parent-&gt;intrinsic_bounds.dim.width) 
            parent-&gt;intrinsic_bounds.dim.width = needed_w;
        if (needed_h &gt; parent-&gt;intrinsic_bounds.dim.height) 
            parent-&gt;intrinsic_bounds.dim.height = needed_h;
    }
}

// --- THE SECOND PASS: The Layout Engine ---
void ui2_layout_compute(Ui2Context* ctx, Ui2BoundingBox screen_bounds) {
    // 1. We iterate through nodes in the order they were created (Top-Down)
    // 2. For each node, we calculate final_bounds based on its parent's space.
    
    for (uint32_
    u32 i = 0; i &lt; ctx-&gt;node_count; ++i) {
        Ui2Node* node = &ctx-&gt;nodes[i];
        Ui2Node* parent = (node-&gt;parent_idx != -1) ? &ctx-&gt;nodes[node-&gt;parent_idx] : NULL;

        // Determine Size
        int32_t target_w = 0;
        int32_t target_h = 0;

        if (!parent) {
            target_w = screen_bounds.dim.width;
            target_h = screen_bounds.dim.height;
        } else {
            // Logic for PERCENT or GROW
            if (parent-&gt;desc.layout.sizing.width.type == UI2_SIZING_TYPE_PERCENT) {
                target_w = (int32_t)(parent-&gt;final_bounds.dim.width * parent-&gt;desc.layout.sizing.width.size.percent);
            } else {
                target_w = node-&gt;intrinsic_bounds.dim.width;
            }
            // ... repeat for height ...
        }

        node-&gt;final_bounds.dim.width = target_w;
        node-&gt;final-&gt;dim.height = target_h;

        // 3. Handle Alignment (Center/Right/Bottom)
        if (parent && parent-&gt;desc.layout.child_alignment.x == UI2_ALIGN_X_CENTER) {
            node-&gt;final_bounds.pos.x = parent-&gt;final_bounds.pos.x + (parent-&gt;final_bounds.dim.width - target_w) / 2;
        } else if (parent) {
            node-&gt;final_bounds.pos.x = parent-&gt;final_bounds.pos.x;
        }

        // 4. Generate Draw Commands for the background
        // (Push to ctx-&gt;draw_cmd_list here)
    }
}
```

### Why this is much better:

1.  **Handling `GROW` and `PERCENT`**: In a single-pass system, you don't know how much space is "left over" until the last child has been processed. With the second pass, you wait until all children are added to the `nodes` array, then you calculate the total used height, subtract it from the parent's total height, and distribute the remainder to any node marked as `GROW`.
2.  **Alignment**: You can't "Center" an element in a single pass because you don't know where the center is until you know how wide the parent is (which depends on all children). The second pass allows you to calculate the parent's total width first, then adjust the $X$ coordinate of all children.
3.  **Decoupling**: Your user code becomes purely "declarative." It simply describes the hierarchy. The `ui2_layout_compute` function acts like a browser's rendering engine (WebKit/Blink), handling the heavy math of Flexbox-style logic separately from the UI logic.

### Complexity Analysis
*   **Time Complexity**: $O(N)$ where $N$ is the number of elements. Even with two passes, it is still linear.
*   **Space Complexity**: $O(N)$ to store the `Ui2Node` array for the frame. Since this is cleared every frame, it's very memory-efficient if using an Arena.

### Recommended Next Step
Implement a **"Cursor"** inside the `ui_layout_compute`. As you iterate through children of a node, keep a `current_y` variable. Every time you place a child, increment `current_y += child_height + gap`. This will implement your `UI2_TOP_TO_BOTTOM` logic perfectly.