#ifndef MEDIT_CORE_RECT_H_
#define MEDIT_CORE_RECT_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
} Point;

typedef struct {
    float x;
    float y;
} PointF;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} Rect;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} RectF;

#define medit_is_point_in_rect(p, r) \
    ((p).x >= (r).x && (p).x < (r).x + (r).w && (p).y >= (r).y && (p).y < (r).y + (r).h)

#define medit_is_point_in_rect_pointers(p, r) \
    ((p)->x >= (r)->x && (p)->x < (r)->x + (r)->w && (p)->y >= (r)->y && (p)->y < (r)->y + (r)->h)

#endif // MEDIT_CORE_RECT_H_
