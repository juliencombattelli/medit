#ifndef MEDIT_CORE_SAFEINT_H_
#define MEDIT_CORE_SAFEINT_H_

#include "assert.h"

#include <limits.h>
#include <stdint.h>

#define size_to_int(i) (assert((i) <= INT_MAX), (int)((i)))

#define int_to_size(i) (assert((i) >= 0), (size_t)((i)))

#endif // MEDIT_CORE_SAFEINT_H_
