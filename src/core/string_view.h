#ifndef MEDIT_CORE_STRING_VIEW_H_
#define MEDIT_CORE_STRING_VIEW_H_

// Inspired from Tsoding's StringView in nob.h
// https://github.com/tsoding/nob.h/blob/v3.8.2/nob.h

#include <core/safeint.h>

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char* data;
    size_t count;
} StringView;

// Create a StringView from a pointer and a length
StringView sv_from(const char* data, size_t count);
// Create a StringView from a null-terminated C string
StringView sv_from_cstr(const char* cstr);

// Chop characters from the left of `sv` while `fn` returns true and returns the chopped portion
typedef bool ChopperFn(char);
StringView sv_chop_while(StringView* sv, ChopperFn* fn);
// Alternate predicate prototype, useful to easily use functions from ctype.h like isalpha()
typedef int ChopperFn2(int);
StringView sv_chop_while2(StringView* sv, ChopperFn2* fn);

// Chop `sv` up to (and consuming) the first occurrence of `delim` and returns the portion before
// the delimiter, or the full view if not found
StringView sv_chop_by_delim(StringView* sv, char delim);
// Chop `sv` up to (and consuming) the first occurrence of any character in `delims` and returns the
// portion before the delimiter, or the full view if none found
StringView sv_chop_by_delims(StringView* sv, const char* delims);

// Chop up to `n` characters from the left of `sv` and returns the chopped portion
StringView sv_chop_left(StringView* sv, size_t n);
// Chop up to `n` characters from the right of `sv`, returns the chopped portion
StringView sv_chop_right(StringView* sv, size_t n);

// If `sv` starts with `prefix` chops off the prefix and returns true; otherwise, leaves `sv`
// unmodified and returns false
bool sv_chop_prefix(StringView* sv, StringView prefix);
// If `sv` ends with `suffix` chops off the suffix and returns true; otherwise, leaves `sv`
// unmodified and returns false
bool sv_chop_suffix(StringView* sv, StringView suffix);

// Trim leading and trailing whitespace from `sv`
StringView sv_trim(StringView sv);
// Trim leading whitespace from `sv`
StringView sv_trim_left(StringView sv);
// Trim trailing whitespace from `sv`
StringView sv_trim_right(StringView sv);

// Return true if `a` and `b` have identical content
bool sv_eq(StringView a, StringView b);

// Return true if `sv` ends with the null-terminated string `suffix`
bool sv_ends_with_cstr(StringView sv, const char* suffix);
// Return true if `sv` ends with `suffix`
bool sv_ends_with_sv(StringView sv, StringView suffix);

// Return true if `sv` starts with the null-terminated string `prefix`
bool sv_starts_with_cstr(StringView sv, const char* prefix);
// Return true if `sv` starts with `prefix`
bool sv_starts_with_sv(StringView sv, StringView prefix);

// printf macros for StringView
// Usage:
//   StringView name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif // SV_Fmt
#ifndef SV_Arg
#define SV_Arg(sv) size_to_int((sv).count), (sv).data
#endif // SV_Arg

// Return the last path component of `path`, stripping any trailing slashes; return "." for an empty
// or all-slash path (POSIX basename semantics)
StringView sv_path_basename(StringView path);

#endif // MEDIT_CORE_STRING_VIEW_H_
