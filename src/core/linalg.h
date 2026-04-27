#ifndef MEDIT_CORE_LINALG_H_
#define MEDIT_CORE_LINALG_H_

#define vec2_add(a, b)                                                                             \
    {                                                                                              \
        (a).v[0] + (b).v[0],                                                                       \
        (a).v[1] + (b).v[1],                                                                       \
    }

#define vec2_sub(a, b)                                                                             \
    {                                                                                              \
        (a).v[0] - (b).v[0],                                                                       \
        (a).v[1] - (b).v[1],                                                                       \
    }

#define vec2_mul(a, b)                                                                             \
    {                                                                                              \
        (a).v[0] * (b).v[0],                                                                       \
        (a).v[1] * (b).v[1],                                                                       \
    }

#define vec2_div(a, b)                                                                             \
    {                                                                                              \
        (a).v[0] / (b).v[0],                                                                       \
        (a).v[1] / (b).v[1],                                                                       \
    }

#endif // MEDIT_CORE_LINALG_H_
