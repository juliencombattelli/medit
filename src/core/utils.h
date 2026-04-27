#ifndef MEDIT_CORE_UTILS_H_
#define MEDIT_CORE_UTILS_H_

#include <limits.h>
#include <stdio.h>
#include <stdlib.h> // IWYU pragma: keep // needed for abort()

#define MEDIT_UNUSED(x) (void)((x))

#define MEDIT_UNREACHABLE(message)                                                                 \
    do {                                                                                           \
        (void)fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);            \
        abort();                                                                                   \
    } while (0)

#define MEDIT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MEDIT_MAX(a, b) (((a) > (b)) ? (a) : (b))

char* medit_strdup(const char* str);

static inline int medit_digits_count(int n)
{
    // NOLINTBEGIN(*magic-numbers*,*braces-around-statement*)
    // clang-format off
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    // clang-format on
    return 10;
    // NOLINTEND(*magic-numbers*,*braces-around-statement*)
}

// Maximum number of digits needed to represent any 64-bits integer (including a potential sign for
// signed integers and a null character)
enum {
    INT64_DIGITS_COUNT = 22u,
};

// Clamp a float to [lo, hi]
static inline float medit_clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

#endif // MEDIT_CORE_UTILS_H_
