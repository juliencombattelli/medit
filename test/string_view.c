#include <core/string_view.h>

#include "test_utils.h"

#include <ctype.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// sv_from / sv_from_cstr
// ---------------------------------------------------------------------------

static void test_sv_from(void)
{
    StringView sv = sv_from("hello", 3);
    CHECK_SV_EQ(sv, "hel", 3);

    StringView sv2 = sv_from_cstr("world");
    CHECK_SV_EQ(sv2, "world", 5);
}

// ---------------------------------------------------------------------------
// sv_chop_while / sv_chop_while2
// ---------------------------------------------------------------------------

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static void test_sv_chop_while(void)
{
    StringView sv = sv_from_cstr("123abc");
    StringView chopped = sv_chop_while(&sv, is_digit);
    CHECK_SV_EQ(chopped, "123", 3);
    CHECK_SV_EQ(sv, "abc", 3);

    // Nothing to chop
    StringView sv2 = sv_from_cstr("abc");
    StringView chopped2 = sv_chop_while(&sv2, is_digit);
    CHECK_SV_EQ(chopped2, "", 0);
    CHECK_SV_EQ(sv2, "abc", 3);

    // Chop everything
    StringView sv3 = sv_from_cstr("999");
    StringView chopped3 = sv_chop_while(&sv3, is_digit);
    CHECK_SV_EQ(chopped3, "999", 3);
    CHECK_SV_EQ(sv3, "", 0);
}

static void test_sv_chop_while2(void)
{
    StringView sv = sv_from_cstr("  \t  hello");
    StringView chopped = sv_chop_while2(&sv, isspace);
    CHECK_EQ(chopped.count, 5u);
    CHECK_SV_EQ(sv, "hello", 5);
}

// ---------------------------------------------------------------------------
// sv_chop_by_delim
// ---------------------------------------------------------------------------

static void test_sv_chop_by_delim(void)
{
    // Delimiter present
    StringView sv = sv_from_cstr("foo:bar");
    StringView token = sv_chop_by_delim(&sv, ':');
    CHECK_SV_EQ(token, "foo", 3);
    CHECK_SV_EQ(sv, "bar", 3);

    // Delimiter absent: returns the full view, leaves sv empty
    StringView sv2 = sv_from_cstr("foobar");
    StringView token2 = sv_chop_by_delim(&sv2, ':');
    CHECK_SV_EQ(token2, "foobar", 6);
    CHECK_EQ(sv2.count, 0u);

    // Delimiter at start: empty token, rest is remainder
    StringView sv3 = sv_from_cstr(":rest");
    StringView token3 = sv_chop_by_delim(&sv3, ':');
    CHECK_EQ(token3.count, 0u);
    CHECK_SV_EQ(sv3, "rest", 4);

    // Delimiter at end: full content token, empty remainder
    StringView sv4 = sv_from_cstr("end:");
    StringView token4 = sv_chop_by_delim(&sv4, ':');
    CHECK_SV_EQ(token4, "end", 3);
    CHECK_EQ(sv4.count, 0u);
}

// ---------------------------------------------------------------------------
// sv_chop_by_delims
// ---------------------------------------------------------------------------

static void test_sv_chop_by_delims(void)
{
    // First delimiter hit
    StringView sv = sv_from_cstr("foo,bar;baz");
    StringView token = sv_chop_by_delims(&sv, ",;");
    CHECK_SV_EQ(token, "foo", 3);
    CHECK_SV_EQ(sv, "bar;baz", 7);

    // Second delimiter hit first
    StringView sv2 = sv_from_cstr("foo;bar,baz");
    StringView token2 = sv_chop_by_delims(&sv2, ",;");
    CHECK_SV_EQ(token2, "foo", 3);
    CHECK_SV_EQ(sv2, "bar,baz", 7);

    // No delimiter found
    StringView sv3 = sv_from_cstr("foobar");
    StringView token3 = sv_chop_by_delims(&sv3, ",;");
    CHECK_SV_EQ(token3, "foobar", 6);
    CHECK_EQ(sv3.count, 0u);

    // Delimiter at start
    StringView sv4 = sv_from_cstr(",rest");
    StringView token4 = sv_chop_by_delims(&sv4, ",;");
    CHECK_EQ(token4.count, 0u);
    CHECK_SV_EQ(sv4, "rest", 4);
}

// ---------------------------------------------------------------------------
// sv_chop_left / sv_chop_right
// ---------------------------------------------------------------------------

static void test_sv_chop_left(void)
{
    StringView sv = sv_from_cstr("hello");
    StringView chopped = sv_chop_left(&sv, 3);
    CHECK_SV_EQ(chopped, "hel", 3);
    CHECK_SV_EQ(sv, "lo", 2);

    // n > count: clamp
    StringView sv2 = sv_from_cstr("hi");
    StringView chopped2 = sv_chop_left(&sv2, 100);
    CHECK_SV_EQ(chopped2, "hi", 2);
    CHECK_EQ(sv2.count, 0u);
}

static void test_sv_chop_right(void)
{
    StringView sv = sv_from_cstr("hello");
    StringView chopped = sv_chop_right(&sv, 2);
    CHECK_SV_EQ(chopped, "lo", 2);
    CHECK_SV_EQ(sv, "hel", 3);

    // n > count: clamp
    StringView sv2 = sv_from_cstr("hi");
    StringView chopped2 = sv_chop_right(&sv2, 100);
    CHECK_SV_EQ(chopped2, "hi", 2);
    CHECK_EQ(sv2.count, 0u);
}

// ---------------------------------------------------------------------------
// sv_chop_prefix / sv_chop_suffix
// ---------------------------------------------------------------------------

static void test_sv_chop_prefix(void)
{
    StringView sv = sv_from_cstr("foobar");
    bool ok = sv_chop_prefix(&sv, sv_from_cstr("foo"));
    CHECK_EQ(ok, 1u);
    CHECK_SV_EQ(sv, "bar", 3);

    // Prefix not present: sv unchanged
    StringView sv2 = sv_from_cstr("foobar");
    bool ok2 = sv_chop_prefix(&sv2, sv_from_cstr("baz"));
    CHECK_EQ(ok2, 0u);
    CHECK_SV_EQ(sv2, "foobar", 6);
}

static void test_sv_chop_suffix(void)
{
    StringView sv = sv_from_cstr("foobar");
    bool ok = sv_chop_suffix(&sv, sv_from_cstr("bar"));
    CHECK_EQ(ok, 1u);
    CHECK_SV_EQ(sv, "foo", 3);

    // Suffix not present: sv unchanged
    StringView sv2 = sv_from_cstr("foobar");
    bool ok2 = sv_chop_suffix(&sv2, sv_from_cstr("baz"));
    CHECK_EQ(ok2, 0u);
    CHECK_SV_EQ(sv2, "foobar", 6);
}

// ---------------------------------------------------------------------------
// sv_trim / sv_trim_left / sv_trim_right
// ---------------------------------------------------------------------------

static void test_sv_trim(void)
{
    StringView sv = sv_trim(sv_from_cstr("  hello  "));
    CHECK_SV_EQ(sv, "hello", 5);

    StringView sv2 = sv_trim_left(sv_from_cstr("  hello  "));
    CHECK_SV_EQ(sv2, "hello  ", 7);

    StringView sv3 = sv_trim_right(sv_from_cstr("  hello  "));
    CHECK_SV_EQ(sv3, "  hello", 7);

    // Already trimmed
    StringView sv4 = sv_trim(sv_from_cstr("hello"));
    CHECK_SV_EQ(sv4, "hello", 5);

    // All whitespace
    StringView sv5 = sv_trim(sv_from_cstr("   "));
    CHECK_EQ(sv5.count, 0u);
}

// ---------------------------------------------------------------------------
// sv_eq
// ---------------------------------------------------------------------------

static void test_sv_eq(void)
{
    CHECK_EQ(sv_eq(sv_from_cstr("abc"), sv_from_cstr("abc")), 1u);
    CHECK_EQ(sv_eq(sv_from_cstr("abc"), sv_from_cstr("abd")), 0u);
    CHECK_EQ(sv_eq(sv_from_cstr("abc"), sv_from_cstr("ab")), 0u);
    CHECK_EQ(sv_eq(sv_from_cstr(""), sv_from_cstr("")), 1u);
}

// ---------------------------------------------------------------------------
// sv_starts_with / sv_ends_with
// ---------------------------------------------------------------------------

static void test_sv_starts_ends_with(void)
{
    CHECK_EQ(sv_starts_with_cstr(sv_from_cstr("foobar"), "foo"), 1u);
    CHECK_EQ(sv_starts_with_cstr(sv_from_cstr("foobar"), "bar"), 0u);
    CHECK_EQ(sv_starts_with_sv(sv_from_cstr("foobar"), sv_from_cstr("foo")), 1u);

    CHECK_EQ(sv_ends_with_cstr(sv_from_cstr("foobar"), "bar"), 1u);
    CHECK_EQ(sv_ends_with_cstr(sv_from_cstr("foobar"), "foo"), 0u);
    CHECK_EQ(sv_ends_with_sv(sv_from_cstr("foobar"), sv_from_cstr("bar")), 1u);

    // Prefix/suffix longer than sv
    CHECK_EQ(sv_starts_with_cstr(sv_from_cstr("hi"), "hello"), 0u);
    CHECK_EQ(sv_ends_with_cstr(sv_from_cstr("hi"), "hello"), 0u);
}

// ---------------------------------------------------------------------------
// sv_path_basename
// ---------------------------------------------------------------------------

static void test_sv_path_basename(void)
{
    // Simple filename
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("file.txt")), "file.txt", 8);

    // Single directory component
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("dir/file.txt")), "file.txt", 8);

    // Deep path
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("/usr/local/bin/prog")), "prog", 4);

    // Trailing slash(es) are ignored
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("dir/sub/")), "sub", 3);
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("dir/sub///")), "sub", 3);

    // Root path: all slashes → "."
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("/")), ".", 1);
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("///")), ".", 1);

    // Empty string → "."
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("")), ".", 1);

    // Path with no slash
    CHECK_SV_EQ(sv_path_basename(sv_from_cstr("nodir")), "nodir", 5);
}

// ---------------------------------------------------------------------------

int main(void)
{
    test_sv_from();
    test_sv_chop_while();
    test_sv_chop_while2();
    test_sv_chop_by_delim();
    test_sv_chop_by_delims();
    test_sv_chop_left();
    test_sv_chop_right();
    test_sv_chop_prefix();
    test_sv_chop_suffix();
    test_sv_trim();
    test_sv_eq();
    test_sv_starts_ends_with();
    test_sv_path_basename();

    if (g_failures == 0) {
        printf("All string_view tests passed.\n");
    } else {
        printf("%d string_view test(s) FAILED.\n", g_failures);
    }
    return g_failures != 0;
}
