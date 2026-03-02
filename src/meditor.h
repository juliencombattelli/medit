#ifndef MEDIT_MEDITOR_H_
#define MEDIT_MEDITOR_H_

#include "color.h"
#include "linalg.h"

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

typedef struct {
    Files opened_files;
    FileViews file_views;
    FileView focused_view;
} Meditor;

extern Meditor medit;

#endif // MEDIT_MEDITOR_H_
