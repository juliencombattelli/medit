#ifndef MEDIT_ASSERT_H_
#define MEDIT_ASSERT_H_

#include <stdio.h>
#include <stdlib.h>

#define assert(expr)                                                                               \
    ((expr) ? (void)0 : ((void)fprintf(stderr, "%s:%u: %s\n", __FILE__, __LINE__, #expr), abort()))

#endif // MEDIT_ASSERT_H_
