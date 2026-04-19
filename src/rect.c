#include "rect.h"

Rect rect_cut_top(Rect* r, size_t amount)
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

Rect rect_cut_bottom(Rect* r, size_t amount)
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

Rect rect_cut_left(Rect* r, size_t amount)
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

Rect rect_cut_right(Rect* r, size_t amount)
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
