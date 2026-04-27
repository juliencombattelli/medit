#include <core/dynarray.h>

#include "test_utils.h"

typedef struct {
    int* items;
    unsigned count;
    unsigned capacity;
} Ints;

int main(void)
{
    Ints ints = { 0 };

    for (int i = 0; i < 5; ++i) {
        dynarray_append(&ints, i);
        CHECK_EQ(ints.items[i], i);
    }

    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(ints.items[i], i);
    }

    dynarray_remove(&ints, 1);
    dynarray_foreach(int, i, &ints)
    {
        printf("%d,", *i);
    }
    printf("\n");
    CHECK_EQ(ints.count, 4u);
    CHECK_EQ(ints.items[0], 0);
    CHECK_EQ(ints.items[1], 2);
    CHECK_EQ(ints.items[2], 3);
    CHECK_EQ(ints.items[3], 4);

    dynarray_remove(&ints, 3);
    CHECK_EQ(ints.count, 3u);
    CHECK_EQ(ints.items[0], 0);
    CHECK_EQ(ints.items[1], 2);
    CHECK_EQ(ints.items[2], 3);

    dynarray_free(ints);
    return g_failures != 0;
}
