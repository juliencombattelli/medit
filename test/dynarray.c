#include <core/dynarray.h>

#include "test_utils.h"

typedef struct {
    int* items;
    unsigned count;
    unsigned capacity;
} Ints;

static void test_append_and_remove(void)
{
    Ints ints = { 0 };

    for (int i = 0; i < 5; ++i) {
        dynarray_append(&ints, i);
        CHECK_EQ(ints.items[i], i);
    }

    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(ints.items[i], i);
    }

    // remove middle element (index 1: value 1)
    dynarray_remove(&ints, 1);
    CHECK_EQ(ints.count, 4u);
    CHECK_EQ(ints.items[0], 0);
    CHECK_EQ(ints.items[1], 2);
    CHECK_EQ(ints.items[2], 3);
    CHECK_EQ(ints.items[3], 4);

    // remove last element
    dynarray_remove(&ints, 3);
    CHECK_EQ(ints.count, 3u);
    CHECK_EQ(ints.items[0], 0);
    CHECK_EQ(ints.items[1], 2);
    CHECK_EQ(ints.items[2], 3);

    dynarray_free(ints);
}

static void test_append_many(void)
{
    Ints ints = { 0 };
    int src[] = { 10, 20, 30 };

    dynarray_append_many(&ints, src, 3);
    CHECK_EQ(ints.count, 3u);
    CHECK_EQ(ints.items[0], 10);
    CHECK_EQ(ints.items[1], 20);
    CHECK_EQ(ints.items[2], 30);

    // append more to an already-populated array
    int src2[] = { 40, 50 };
    dynarray_append_many(&ints, src2, 2);
    CHECK_EQ(ints.count, 5u);
    CHECK_EQ(ints.items[3], 40);
    CHECK_EQ(ints.items[4], 50);

    dynarray_free(ints);
}

static void test_insert(void)
{
    Ints ints = { 0 };
    dynarray_append(&ints, 1);
    dynarray_append(&ints, 3);

    // insert 2 between 1 and 3
    dynarray_insert(&ints, 2, 1);
    CHECK_EQ(ints.count, 3u);
    CHECK_EQ(ints.items[0], 1);
    CHECK_EQ(ints.items[1], 2);
    CHECK_EQ(ints.items[2], 3);

    // insert at the beginning
    dynarray_insert(&ints, 0, 0);
    CHECK_EQ(ints.count, 4u);
    CHECK_EQ(ints.items[0], 0);
    CHECK_EQ(ints.items[1], 1);

    dynarray_free(ints);
}

static void test_insert_many(void)
{
    Ints ints = { 0 };
    dynarray_append(&ints, 1);
    dynarray_append(&ints, 5);

    int mid[] = { 2, 3, 4 };
    dynarray_insert_many(&ints, mid, 3, 1);
    CHECK_EQ(ints.count, 5u);
    CHECK_EQ(ints.items[0], 1);
    CHECK_EQ(ints.items[1], 2);
    CHECK_EQ(ints.items[2], 3);
    CHECK_EQ(ints.items[3], 4);
    CHECK_EQ(ints.items[4], 5);

    dynarray_free(ints);
}

static void test_last(void)
{
    Ints ints = { 0 };
    dynarray_append(&ints, 7);
    dynarray_append(&ints, 42);
    CHECK_EQ(dynarray_last(&ints), 42);

    dynarray_free(ints);
}

static void test_remove_unordered(void)
{
    Ints ints = { 0 };
    dynarray_append(&ints, 10);
    dynarray_append(&ints, 20);
    dynarray_append(&ints, 30);

    // remove index 1: last element (30) takes its place
    dynarray_remove_unordered(&ints, 1);
    CHECK_EQ(ints.count, 2u);
    CHECK_EQ(ints.items[0], 10);
    CHECK_EQ(ints.items[1], 30);

    dynarray_free(ints);
}

static void test_remove_many(void)
{
    Ints ints = { 0 };
    for (int i = 0; i < 6; ++i) {
        dynarray_append(&ints, i);
    }

    // remove 2 elements starting at index 2 (removes 2 and 3)
    dynarray_remove_many(&ints, 2, 2);
    CHECK_EQ(ints.count, 4u);
    CHECK_EQ(ints.items[0], 0);
    CHECK_EQ(ints.items[1], 1);
    CHECK_EQ(ints.items[2], 4);
    CHECK_EQ(ints.items[3], 5);

    dynarray_free(ints);
}

static void test_resize(void)
{
    Ints ints = { 0 };
    dynarray_resize(&ints, 3);
    CHECK_EQ(ints.count, 3u);

    ints.items[0] = 1;
    ints.items[1] = 2;
    ints.items[2] = 3;

    // shrink
    dynarray_resize(&ints, 1);
    CHECK_EQ(ints.count, 1u);
    CHECK_EQ(ints.items[0], 1);

    dynarray_free(ints);
}

static void test_foreach(void)
{
    Ints ints = { 0 };
    for (int i = 0; i < 4; ++i) {
        dynarray_append(&ints, i * 2);
    }

    int sum = 0;
    dynarray_foreach(int, it, &ints)
    {
        sum += *it;
    }
    // 0 + 2 + 4 + 6 = 12
    CHECK_EQ(sum, 12);

    dynarray_free(ints);
}

int main(void)
{
    test_append_and_remove();
    test_append_many();
    test_insert();
    test_insert_many();
    test_last();
    test_remove_unordered();
    test_remove_many();
    test_resize();
    test_foreach();

    if (g_failures == 0) {
        printf("All dynarray tests passed.\n");
    } else {
        printf("%d dynarray test(s) FAILED.\n", g_failures);
    }
    return g_failures != 0;
}
