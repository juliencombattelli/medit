#include "utils.h"

#include <stdlib.h>
#include <string.h>

char* medit_strdup(const char* str)
{
    size_t size = strlen(str) + 1;
    char* dest = malloc(size);
    if (dest) {
        memcpy(dest, str, size);
    }
    return dest;
}
