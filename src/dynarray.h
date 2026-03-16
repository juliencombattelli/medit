#ifndef MEDIT_DYNARRAY_H_
#define MEDIT_DYNARRAY_H_

#include "assert.h"

#include <string.h>

#ifdef __cplusplus
#define MEDIT_DECLTYPE_CAST(T) (decltype(T))
#else
#define MEDIT_DECLTYPE_CAST(T)
#endif // __cplusplus

#define DYNARRAY_INITIAL_CAPACITY 256

#define dynarray_reserve(da, expected_capacity)                                                    \
    do {                                                                                           \
        if ((expected_capacity) > (da)->capacity) {                                                \
            if ((da)->capacity == 0) {                                                             \
                (da)->capacity = DYNARRAY_INITIAL_CAPACITY;                                        \
            }                                                                                      \
            while ((expected_capacity) > (da)->capacity) {                                         \
                (da)->capacity *= 2;                                                               \
            }                                                                                      \
            void* ptr = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));               \
            assert(ptr != NULL && "Buy more RAM lol");                                             \
            (da)->items = MEDIT_DECLTYPE_CAST((da)->items)(ptr);                                   \
        }                                                                                          \
    } while (0)

#define dynarray_free(da) free((da).items)

#define dynarray_resize(da, new_size)                                                              \
    do {                                                                                           \
        dynarray_reserve((da), (new_size));                                                        \
        (da)->count = (new_size);                                                                  \
    } while (0)

#define dynarray_append(da, item)                                                                  \
    do {                                                                                           \
        dynarray_reserve((da), (da)->count + 1);                                                   \
        (da)->items[(da)->count++] = (item);                                                       \
    } while (0)

#define dynarray_append_many(da, new_items, new_items_count)                                       \
    do {                                                                                           \
        dynarray_reserve((da), (da)->count + (new_items_count));                                   \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count) * sizeof(*(da)->items));  \
        (da)->count += (new_items_count);                                                          \
    } while (0)

#define dynarray_insert(da, item, index)                                                           \
    do {                                                                                           \
        dynarray_resize((da), (da)->count + 1);                                                    \
        memmove(                                                                                   \
            (da)->items + (index) + 1,                                                             \
            (da)->items + (index),                                                                 \
            ((da)->count - 1 - (index)) * sizeof(*(da)->items));                                   \
        (da)->items[(index)] = (item);                                                             \
    } while (0)

#define dynarray_insert_many(da, new_items, new_items_count, index)                                \
    do {                                                                                           \
        dynarray_resize((da), (da)->count + (new_items_count));                                    \
        memmove(                                                                                   \
            (da)->items + (index) + (new_items_count),                                             \
            (da)->items + (index),                                                                 \
            ((da)->count - (new_items_count) - (index)) * sizeof(*(da)->items));                   \
        memcpy((da)->items + (index), (new_items), (new_items_count) * sizeof(*(da)->items));      \
    } while (0)

#define dynarray_last(da) (da)->items[(assert((da)->count > 0), (da)->count - 1)]

#define dynarray_remove_unordered(da, i)                                                           \
    do {                                                                                           \
        size_t j = (i);                                                                            \
        assert(j < (da)->count);                                                                   \
        (da)->count -= 1;                                                                          \
        (da)->items[j] = (da)->items[(da)->count];                                                 \
    } while (0)

#define dynarray_remove(da, pos)                                                                   \
    do {                                                                                           \
        assert(pos < (da)->count);                                                                 \
        memmove(                                                                                   \
            (da)->items + (pos),                                                                   \
            (da)->items + (pos) + 1,                                                               \
            ((da)->count - 1 - (pos)) * sizeof(*(da)->items));                                     \
        (da)->count -= 1;                                                                          \
    } while (0)

#define dynarray_foreach(Type, it, da)                                                             \
    for (Type* it = (da)->items; it < (da)->items + (da)->count; ++it)

#endif // MEDIT_ALLOC_H_
