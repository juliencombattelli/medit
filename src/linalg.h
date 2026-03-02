#ifndef MEDIT_LINALG_H_
#define MEDIT_LINALG_H_

typedef struct {
    int x;
    int y;
} Vec2;

static inline Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return (Vec2) {
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

static inline Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return (Vec2) {
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
}

static inline Vec2 vec2_mul(Vec2 a, Vec2 b)
{
    return (Vec2) {
        .x = a.x * b.x,
        .y = a.y * b.y,
    };
}

static inline Vec2 vec2_div(Vec2 a, Vec2 b)
{
    return (Vec2) {
        .x = a.x / b.x,
        .y = a.y / b.y,
    };
}

static inline Vec2 vec2_scal_mul(Vec2 a, int b)
{
    return (Vec2) {
        .x = a.x * b,
        .y = a.y * b,
    };
}

static inline Vec2 vec2_scal_div(Vec2 a, int b)
{
    return (Vec2) {
        .x = a.x / b,
        .y = a.y / b,
    };
}

#endif // MEDIT_LINALG_H_
