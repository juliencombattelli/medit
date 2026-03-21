#ifndef MEDIT_SAFEINT_H_
#define MEDIT_SAFEINT_H_

#include "assert.h"

#include <limits.h>
#include <stdint.h>

static inline int size_to_int(size_t i)
{
    assert(i <= INT_MAX);
    return (int)i;
}

#endif // MEDIT_SAFEINT_H_
