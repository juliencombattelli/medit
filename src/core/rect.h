#ifndef MEDIT_RECT_H_
#define MEDIT_RECT_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t x;
    size_t y;
    size_t w;
    size_t h;
} Rect;

Rect rect_cut_top(Rect* r, size_t amount);
Rect rect_cut_bottom(Rect* r, size_t amount);
Rect rect_cut_left(Rect* r, size_t amount);
Rect rect_cut_right(Rect* r, size_t amount);

bool rect_contains(Rect r, size_t x, size_t y);

typedef struct {
    Rect area;
    Rect separator;
} Panel;

Panel panel_cut_top(Rect* r, size_t height, size_t sep);
Panel panel_cut_bottom(Rect* r, size_t height, size_t sep);
Panel panel_cut_left(Rect* r, size_t width, size_t sep);
Panel panel_cut_right(Rect* r, size_t width, size_t sep);

#endif // MEDIT_RECT_H_
