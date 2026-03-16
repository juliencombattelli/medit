#include "dynarray.h"

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
        assert(ints.items[i] == i);
    }

    for (int i = 0; i < 5; ++i) {
        assert(ints.items[i] == i);
    }

    dynarray_remove(&ints, 1);
    dynarray_foreach(int, i, &ints)
    {
        printf("%d,", *i);
    }
    printf("\n");
    assert(ints.count == 4);
    assert(ints.items[0] == 0);
    assert(ints.items[1] == 2);
    assert(ints.items[2] == 3);
    assert(ints.items[3] == 4);

    dynarray_remove(&ints, 3);
    assert(ints.count == 3);
    assert(ints.items[0] == 0);
    assert(ints.items[1] == 2);
    assert(ints.items[2] == 3);

    dynarray_free(ints);
}
