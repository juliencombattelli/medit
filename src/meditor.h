#ifndef MEDIT_MEDITOR_H_
#define MEDIT_MEDITOR_H_

#include "keybind.h"
#include "types.h"

#include <stdbool.h>

enum {
    FONT_SIZE_MIN = 2,
    FONT_SIZE_MAX = 128,
    FONT_SIZE_DEFAULT = 20,
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
    Cell* items;
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
} FileViews;

typedef struct {
    Color editor_fg;
    Color editor_bg;
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
    FileViews file_views;
    FileView focused_view;
    Cell grid_size;
    bool running;
    bool input_in_frame;
    bool draw_debug_grid;
} Meditor;

void meditor_load_default_gui_keybind(Meditor* medit);
void meditor_load_default_tui_keybind(Meditor* medit);

void meditor_cursor_up(Meditor* medit, size_t cells);
void meditor_cursor_down(Meditor* medit, size_t cells);
void meditor_cursor_left(Meditor* medit, size_t cells);
void meditor_cursor_right(Meditor* medit, size_t cells);

void meditor_insert_text(Meditor* medit, const char* text, size_t n, size_t cells);

void meditor_new_empty_file(Meditor* medit);
void meditor_close_files(Meditor* medit);

void meditor_split_line(Meditor* medit);
void meditor_insert_new_line(Meditor* medit);

Line* meditor_get_current_line(Meditor* medit);

void meditor_erase_char(Meditor* medit);
void meditor_erase_line(Meditor* medit);

#endif // MEDIT_MEDITOR_H_
