#ifndef MEDIT_MEDITOR_H_
#define MEDIT_MEDITOR_H_

#include "keybind.h"
#include "linalg.h"
#include "renderer.h"

#include <stdbool.h>

enum {
    FONT_SIZE_MIN = 2,
    FONT_SIZE_MAX = 128,
    FONT_SIZE_DEFAULT = 20,
};

#define FONT_PATH_DEFAULT "asset/font/consola.ttf"

typedef struct {
    char* items;
    int count;
    int capacity;
} Line;

typedef struct {
    Line* items;
    int count;
    int capacity;
} Lines;

typedef struct {
    const char* name;
    Lines lines;
} File;

typedef struct {
    File* items;
    int count;
    int capacity;
} Files;

typedef struct {
    File* file;
    Vec2 cursor;
} FileView;

typedef struct {
    FileView* items;
    int count;
    int capacity;
} FileViews;

typedef struct {
    int window_width;
    int window_height;
    int editor_font_size;
    const char* editor_font_path;
} Config;

typedef struct Meditor {
    Renderer renderer;
    Config startup_config;
    Keybind keybind;
    Files opened_files;
    FileViews file_views;
    FileView focused_view;
    size_t cursor_index;
    Vec2 cursor_pos[16]; // TODO create it dynamically
    Vec2 grid_size;
    const char* editor_font_path;
    int editor_font_size; // TODO switch to size_t
    bool running;
    bool input_in_frame;
    bool draw_debug_grid;
} Meditor;

void meditor_load_default_gui_keybind(Meditor* medit);
void meditor_load_default_tui_keybind(Meditor* medit);

void meditor_cursor_up(Meditor* medit, int cells);
void meditor_cursor_down(Meditor* medit, int cells);
void meditor_cursor_left(Meditor* medit, int cells);
void meditor_cursor_right(Meditor* medit, int cells);

void meditor_insert_text(Meditor* medit, const char* text, int n, int cells);

void meditor_new_file(Meditor* medit);

void meditor_split_line(Meditor* medit);
void meditor_insert_new_line(Meditor* medit);

Line* meditor_get_current_line(Meditor* medit);

void meditor_erase_char(Meditor* medit);
void meditor_erase_line(Meditor* medit);

#endif // MEDIT_MEDITOR_H_
