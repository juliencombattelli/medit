#include "rect.h"

Rect rect_cut_top(Rect* r, int32_t amount)
{
    if (amount > r->h) {
        amount = r->h;
    }
    Rect cut = {
        .x = r->x,
        .y = r->y,
        .w = r->w,
        .h = amount,
    };
    r->y += amount;
    r->h -= amount;
    return cut;
}

Rect rect_cut_bottom(Rect* r, int32_t amount)
{
    if (amount > r->h) {
        amount = r->h;
    }
    r->h -= amount;
    return (Rect) {
        .x = r->x,
        .y = r->y + r->h,
        .w = r->w,
        .h = amount,
    };
}

Rect rect_cut_left(Rect* r, int32_t amount)
{
    if (amount > r->w) {
        amount = r->w;
    }
    Rect cut = {
        .x = r->x,
        .y = r->y,
        .w = amount,
        .h = r->h,
    };
    r->x += amount;
    r->w -= amount;
    return cut;
}

Rect rect_cut_right(Rect* r, int32_t amount)
{
    if (amount > r->w) {
        amount = r->w;
    }
    r->w -= amount;
    return (Rect) {
        .x = r->x + r->w,
        .y = r->y,
        .w = amount,
        .h = r->h,
    };
}

bool rect_contains(Rect r, int32_t x, int32_t y)
{
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

Panel panel_cut_top(Rect* r, int32_t height, int32_t sep)
{
    return (Panel) {
        .area = rect_cut_top(r, height),
        .separator = rect_cut_top(r, sep),
    };
}

Panel panel_cut_bottom(Rect* r, int32_t height, int32_t sep)
{
    return (Panel) {
        .area = rect_cut_bottom(r, height),
        .separator = rect_cut_bottom(r, sep),
    };
}

Panel panel_cut_left(Rect* r, int32_t width, int32_t sep)
{
    return (Panel) {
        .area = rect_cut_left(r, width),
        .separator = rect_cut_left(r, sep),
    };
}

Panel panel_cut_right(Rect* r, int32_t width, int32_t sep)
{
    return (Panel) {
        .area = rect_cut_right(r, width),
        .separator = rect_cut_right(r, sep),
    };
}
