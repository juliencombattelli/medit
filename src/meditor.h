#ifndef MEDIT_MEDITOR_H_
#define MEDIT_MEDITOR_H_

#include "color.h"
#include "keybind.h"

#include <stdbool.h>

enum {
    FONT_SIZE_MIN = 2,
    FONT_SIZE_MAX = 128,
    FONT_SIZE_DEFAULT = 14,
    FONT_DPI_DEFAULT = 96,
};

#define FONT_PATH_DEFAULT "asset/font/consola.ttf"

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
} File;

typedef struct {
    File* items;
    size_t count;
    size_t capacity;
} Files;

typedef struct {
    size_t x;
    size_t y;
    size_t w;
    size_t h;
} Rect;

typedef struct {
    size_t line;
    size_t byte;
    size_t len; // length in bytes of the grapheme under the cursor
    // Grapheme column from the start of the line, set by horizontal moves and preserved across
    // vertical moves so that up/down navigation lands on the correct grapheme even when lines
    // contain multi-byte clusters.
    size_t preferred_col;
    // On screen coordinates of the cursor, excluding the ui elements around the file view
    // So (x=0,y=0) is the top-left corner if the first text glyph in of the viewed file
    Rect on_screen;
} Cursor;

typedef struct {
    Cursor* items;
    size_t count;
    size_t capacity;
} Cursors;

typedef struct {
    size_t x;
    size_t y;
} Scrolling;

typedef struct {
    File* file;
    Cursors cursors;
    Scrolling scrolling;
} FileView;

typedef struct {
    FileView* items;
    size_t count;
    size_t capacity;
    size_t displayed;
    Rect area; // area covered by the group on screen
} FileViewGroup;

typedef struct {
    FileViewGroup* items;
    size_t count;
    size_t capacity;
    size_t focused;
} FileViewGroups;

typedef struct {
    Color editor_fg;
    Color editor_bg;
    Color line_number;
    Color line_number_current;
    Color sidebar_bg;
    Color cursor;
} ColorTheme;

typedef struct {
    int window_width;
    int window_height;
    int editor_font_size;
    const char* editor_font_path;
    ColorTheme color_theme;
} Config;

typedef struct Meditor {
    Config config;
    Keybind keybind;
    Files opened_files;
    FileViewGroups file_views;
    bool running;
} Meditor;

// TODO move into UI modules
void medit_load_default_gui_keybind(Meditor* medit);
void medit_load_default_tui_keybind(Meditor* medit);

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
void medit_close_files(Meditor* medit);

void medit_split_line_at_cursor(Meditor* medit);
Line* medit_new_line_at(Meditor* medit, size_t pos);

FileViewGroup* medit_get_focused_file_view_group(Meditor* medit);
FileView* medit_get_displayed_file_view_in_group(Meditor* medit, FileViewGroup* group);
FileView* medit_get_focused_file_view(Meditor* medit);

// Get the line at main cursor in the focused file view
Line* medit_get_current_line(Meditor* medit);

void medit_erase_char(Meditor* medit);
void medit_erase_line(Meditor* medit);

#endif // MEDIT_MEDITOR_H_
