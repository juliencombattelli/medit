#ifndef MEDIT_CORE_RECT_H_
#define MEDIT_CORE_RECT_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} Rect;

Rect rect_cut_top(Rect* r, int32_t amount);
Rect rect_cut_bottom(Rect* r, int32_t amount);
Rect rect_cut_left(Rect* r, int32_t amount);
Rect rect_cut_right(Rect* r, int32_t amount);

bool rect_contains(Rect r, int32_t x, int32_t y);

typedef struct {
    Rect area;
    Rect separator;
} Panel;

Panel panel_cut_top(Rect* r, int32_t height, int32_t sep);
Panel panel_cut_bottom(Rect* r, int32_t height, int32_t sep);
Panel panel_cut_left(Rect* r, int32_t width, int32_t sep);
Panel panel_cut_right(Rect* r, int32_t width, int32_t sep);

#endif // MEDIT_CORE_RECT_H_
