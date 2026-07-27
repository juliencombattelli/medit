#ifndef MEDIT_CORE_ASSERT_H_
#define MEDIT_CORE_ASSERT_H_

#include <stdio.h>
#include <stdlib.h> // IWYU pragma: keep // needed for abort()

#define assert(expr)                                                                               \
    ((expr) ? (void)0 : ((void)fprintf(stderr, "%s:%u: %s\n", __FILE__, __LINE__, #expr), abort()))

#define assert_sdl(expr)                                                                               \
    ((expr) ? (void)0 : ((void)fprintf(stderr, "%s:%u: %s: SDL Error: %s\n", __FILE__, __LINE__, #expr, SDL_GetError()), abort()))

#endif // MEDIT_CORE_ASSERT_H_
