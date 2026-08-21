#ifndef MEDIT_CORE_MEDITOR_H_
#define MEDIT_CORE_MEDITOR_H_

#include "action.h"
#include "keybind.h"
#include <core/ui/color.h>
#include <core/ui/rect.h>
#include <core/ui/ui.h>

#include <stdbool.h>

enum {
    FONT_SIZE_MIN = 2,
    FONT_SIZE_MAX = 128,
    FONT_SIZE_DEFAULT = 14,
    FONT_DPI_DEFAULT = 96,
};

#define FONT_PATH_DEFAULT "asset/font/consola.ttf"

int clamp_font_size(int size);

typedef enum {
    MEDIT_STATUS_SUCCESS,
    MEDIT_STATUS_FAILURE,
    MEDIT_STATUS_FAILED_TO_CREATE_GUI,
} MeditStatusCode;

typedef struct {
    char* items;
    size_t count;
    size_t capacity;
} Line;

typedef struct {
    Line* items;
    size_t count;
    size_t capacity;
} Lines;

typedef struct {
    const char* name;
    Lines lines;
    size_t untitled_number;
    bool dirty;
} File;

typedef struct {
    File* items;
    size_t count;
    size_t capacity;
} Files;

typedef struct {
    size_t line; // line number where the cursor is (0 based)
    size_t byte; // byte in the line where the cursor is (0 based)
    size_t grapheme_col; // grapheme number in the line where the cursor is (0 based)
    size_t len; // length in bytes of the grapheme under the cursor
    // Grapheme column from the start of the line, set by horizontal moves and preserved across
    // vertical moves so that up/down navigation lands on the correct grapheme even when lines
    // contain multi-byte clusters. (0 based)
    size_t preferred_col;
} Cursor;

typedef struct {
    Cursor* items;
    size_t count;
    size_t capacity;
} Cursors;

typedef struct {
    int32_t x;
    int32_t y;
} Scrolling;

typedef struct {
    size_t file_index;
    Cursors cursors;
    Scrolling scrolling;
} FileView;

typedef struct {
    FileView* items;
    size_t count;
    size_t capacity;
    size_t displayed;
    Rect area; // area covered by the group on screen (including tab bar)
    Rect content_area; // area for editor content (area minus tab bar)
} FileViewGroup;

typedef struct {
    FileViewGroup* items;
    size_t count;
    size_t capacity;
    size_t focused;
} FileViewGroups;

typedef struct {
    int window_width;
    int window_height;
    int editor_font_size;
    const char* editor_font_path;
    ColorScheme color_scheme;
} Config;

typedef struct Meditor {
    Config config;
    Keybind keybind;
    Files opened_files;
    FileViewGroups file_views;
    bool running;
} Meditor;

void medit_cursor_up(Meditor* medit);
void medit_cursor_down(Meditor* medit);
void medit_cursor_left(Meditor* medit);
void medit_cursor_right(Meditor* medit);
void medit_cursor_line_begin(Meditor* medit);
void medit_cursor_line_end(Meditor* medit);
void medit_cursor_file_begin(Meditor* medit);
void medit_cursor_file_end(Meditor* medit);

void medit_insert_text(Meditor* medit, const char* text, size_t n);

void medit_new_empty_file(Meditor* medit, FileViewGroup* group);
void medit_load_file(Meditor* medit, const char* filepath);
void medit_save_file(File* file);
void medit_save_focused_file(Meditor* medit);
void medit_close_file(File* file);
void medit_close_all_files(Meditor* medit);

File* medit_file_view_file(Meditor* medit, FileView* fv);
void medit_file_view_group_display_next(Meditor* medit, FileViewGroup* group);
void medit_file_view_group_display_prev(Meditor* medit, FileViewGroup* group);

void medit_split_line_at_cursor(Meditor* medit);
Line* medit_new_line_at(Meditor* medit, size_t pos);

FileViewGroup* medit_get_focused_file_view_group(Meditor* medit);
FileView* medit_get_displayed_file_view_in_group(Meditor* medit, FileViewGroup* group);
FileView* medit_get_focused_file_view(Meditor* medit);

// Get the line at main cursor in the focused file view
Line* medit_get_current_line(Meditor* medit);

void medit_erase_char(Meditor* medit);
void medit_erase_line(Meditor* medit);

void medit_dump_state(Meditor* medit);

// Load the default keybindings for the associated UI type:
// Full: graphical UIs where all key+modifiers combinations are distinguishable.
// ANSI: terminal UIs where CTRL+letter and CTRL+SHIFT+letter produce the same control character
//       limiting available bindings.
void medit_load_default_keybind_full(Meditor* medit, const Actions* actions, void* ui);
void medit_load_default_keybind_ansi(Meditor* medit, const Actions* actions, void* ui);

#endif // MEDIT_CORE_MEDITOR_H_
