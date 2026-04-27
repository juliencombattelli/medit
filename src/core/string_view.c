#include "string_view.h"

#include <ctype.h>
#include <string.h>

StringView sv_from(const char* data, size_t count)
{
    return (StringView) {
        .data = data,
        .count = count,
    };
}

StringView sv_from_cstr(const char* cstr)
{
    return sv_from(cstr, strlen(cstr));
}

StringView sv_chop_while(StringView* sv, ChopperFn* fn)
{
    size_t i = 0;
    while (i < sv->count && fn(sv->data[i])) {
        i += 1;
    }

    StringView result = sv_from(sv->data, i);
    sv->count -= i;
    sv->data += i;

    return result;
}

StringView sv_chop_while2(StringView* sv, ChopperFn2* fn)
{
    size_t i = 0;
    while (i < sv->count && fn(sv->data[i])) {
        i += 1;
    }

    StringView result = sv_from(sv->data, i);
    sv->count -= i;
    sv->data += i;

    return result;
}

StringView sv_chop_by_delim(StringView* sv, char delim)
{
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    StringView result = sv_from(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data += i + 1;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

StringView sv_chop_by_delims(StringView* sv, const char* delims)
{
    size_t i = 0;
    while (i < sv->count && !strchr(delims, sv->data[i])) {
        i += 1;
    }

    StringView result = sv_from(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data += i + 1;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

StringView sv_chop_left(StringView* sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    StringView result = sv_from(sv->data, n);

    sv->data += n;
    sv->count -= n;

    return result;
}

StringView sv_chop_right(StringView* sv, size_t n)
{
    if (n > sv->count) {
        n = sv->count;
    }

    StringView result = sv_from(sv->data + sv->count - n, n);

    sv->count -= n;

    return result;
}

bool sv_chop_prefix(StringView* sv, StringView prefix)
{
    if (sv_starts_with_sv(*sv, prefix)) {
        sv_chop_left(sv, prefix.count);
        return true;
    }
    return false;
}

bool sv_chop_suffix(StringView* sv, StringView suffix)
{
    if (sv_ends_with_sv(*sv, suffix)) {
        sv_chop_right(sv, suffix.count);
        return true;
    }
    return false;
}

StringView sv_trim(StringView sv)
{
    return sv_trim_right(sv_trim_left(sv));
}

StringView sv_trim_left(StringView sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return sv_from(sv.data + i, sv.count - i);
}

StringView sv_trim_right(StringView sv)
{
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return sv_from(sv.data, sv.count - i);
}

bool sv_eq(StringView a, StringView b)
{
    if (a.count != b.count) {
        return false;
    }
    return memcmp(a.data, b.data, a.count) == 0;
}

bool sv_ends_with_cstr(StringView sv, const char* suffix)
{
    return sv_ends_with_sv(sv, sv_from_cstr(suffix));
}

bool sv_ends_with_sv(StringView sv, StringView suffix)
{
    if (sv.count >= suffix.count) {
        StringView sv_tail = sv_from(sv.data + sv.count - suffix.count, suffix.count);
        return sv_eq(sv_tail, suffix);
    }
    return false;
}

bool sv_starts_with_cstr(StringView sv, const char* prefix)
{
    return sv_starts_with_sv(sv, sv_from_cstr(prefix));
}

bool sv_starts_with_sv(StringView sv, StringView prefix)
{
    if (prefix.count <= sv.count) {
        StringView sv_head = sv_from(sv.data, prefix.count);
        return sv_eq(sv_head, prefix);
    }

    return false;
}
