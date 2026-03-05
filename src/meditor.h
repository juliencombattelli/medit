#ifndef MEDIT_MEDITOR_H_
#define MEDIT_MEDITOR_H_

#include "linalg.h"

#include <stdbool.h>
#include <stdlib.h>

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
    File* file;
    Vec2 cursor;
} FileView;

typedef struct {
    FileView* items;
    size_t count;
    size_t capacity;
} FileViews;

#define TEXT_CAPACITY (size_t)(1024 * 1024)

typedef struct {
    // Files opened_files;
    // FileViews file_views;
    // FileView focused_view;
    int cursor_col;
    int cursor_row;
    int grid_cols;
    int grid_rows;
    bool draw_debug_grid;
    size_t text_size;
    char text[TEXT_CAPACITY];
} Meditor;

void meditor_cursor_up(Meditor* medit, int cells);
void meditor_cursor_down(Meditor* medit, int cells);
void meditor_cursor_left(Meditor* medit, int cells);
void meditor_cursor_right(Meditor* medit, int cells);

void meditor_append_text(Meditor* medit, const char* text);

#endif // MEDIT_MEDITOR_H_
