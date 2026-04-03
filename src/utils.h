#ifndef MEDIT_UTILS_H_
#define MEDIT_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#define MEDIT_UNUSED(x) (void)((x))

#define MEDIT_UNREACHABLE(message)                                                                 \
    do {                                                                                           \
        (void)fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);            \
        abort();                                                                                   \
    } while (0)

#define MEDIT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MEDIT_MAX(a, b) (((a) > (b)) ? (a) : (b))

char* medit_strdup(const char* str);

#endif // MEDIT_UTILS_H_
