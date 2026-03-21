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
    size_t line;
    size_t byte;
} Cursor;

typedef struct {
    Cursor* items;
    size_t count;
    size_t capacity;
} Cursors;

typedef struct {
    File* file;
    Cursors cursors;
} FileView;

typedef struct {
    FileView* items;
    size_t count;
    size_t capacity;
    size_t displayed;
    size_t offset;
    size_t width;
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
    bool input_in_frame;
} Meditor;

// TODO move into UI modules
void medit_load_default_gui_keybind(Meditor* medit);
void medit_load_default_tui_keybind(Meditor* medit);

void medit_cursor_up(Meditor* medit);
void medit_cursor_down(Meditor* medit);
void medit_cursor_left(Meditor* medit);
void medit_cursor_right(Meditor* medit);

void medit_insert_text(Meditor* medit, const char* text, size_t n);

void medit_new_empty_file(Meditor* medit);
void medit_close_files(Meditor* medit);

void medit_split_line(Meditor* medit);
void medit_insert_new_line(Meditor* medit);

FileViewGroup* medit_get_focused_file_view_group(Meditor* medit);
FileView* medit_get_displayed_file_view_in_group(Meditor* medit, FileViewGroup* group);
FileView* medit_get_focused_file_view(Meditor* medit);

// Get the line at main cursor in the focused file view
Line* medit_get_current_line(Meditor* medit);

void medit_erase_char(Meditor* medit);
void medit_erase_line(Meditor* medit);

#endif // MEDIT_MEDITOR_H_
