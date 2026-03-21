#ifndef MEDIT_UTILS_H_
#define MEDIT_UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#define MEDIT_UNUSED(x) (void)((x))

#define MEDIT_UNREACHABLE(message)                                                                 \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);                  \
        abort();                                                                                   \
    } while (0)

#define MEDIT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MEDIT_MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif // MEDIT_UTILS_H_
