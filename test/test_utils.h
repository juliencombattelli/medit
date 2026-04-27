#ifndef MEDIT_TEST_UTILS_H_
#define MEDIT_TEST_UTILS_H_

#include <stdio.h>
#include <string.h> // IWYU pragma: keep

static int g_failures = 0;

#define CHECK_EQ(a, b)                                                                             \
    do {                                                                                           \
        size_t _a = (size_t)(a);                                                                   \
        size_t _b = (size_t)(b);                                                                   \
        if (_a != _b) {                                                                            \
            (void)fprintf(                                                                         \
                stderr,                                                                            \
                "%s:%d: FAIL  %s == %s  (%zu != %zu)\n",                                           \
                __FILE__,                                                                          \
                __LINE__,                                                                          \
                #a,                                                                                \
                #b,                                                                                \
                _a,                                                                                \
                _b);                                                                               \
            ++g_failures;                                                                          \
        }                                                                                          \
    } while (0)

#define CHECK_SV_EQ(sv, expected_data, expected_count)                                             \
    do {                                                                                           \
        CHECK_EQ((sv).count, (size_t)(expected_count));                                            \
        if ((sv).count == (size_t)(expected_count)) {                                              \
            if (memcmp((sv).data, (expected_data), (size_t)(expected_count)) != 0) {               \
                (void)fprintf(                                                                     \
                    stderr,                                                                        \
                    "%s:%d: FAIL  sv content mismatch: got \"%.*s\", expected \"%s\"\n",           \
                    __FILE__,                                                                      \
                    __LINE__,                                                                      \
                    (int)(sv).count,                                                               \
                    (sv).data,                                                                     \
                    (expected_data));                                                              \
                ++g_failures;                                                                      \
            }                                                                                      \
        }                                                                                          \
    } while (0)

#endif // MEDIT_TEST_UTILS_H_
