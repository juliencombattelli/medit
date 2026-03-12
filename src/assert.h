#ifndef MEDIT_ASSERT_H_
#define MEDIT_ASSERT_H_

#include <stdio.h>
#include <stdlib.h>

#define assert(EXPR)                                                                               \
    do {                                                                                           \
        if (!(EXPR)) {                                                                             \
            (void)fprintf(stderr, "%s:%u: %s\n", __FILE__, __LINE__, #EXPR);                       \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

#endif // MEDIT_ASSERT_H_
