#ifndef MEDIT_UNICODE_ITER_H_
#define MEDIT_UNICODE_ITER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A half-open byte range [start, start+len) within a string.
typedef struct {
    size_t start;
    size_t len;
} UcSpan;

// Grapheme cluster iterator
//
// Iterates over user-perceived characters (Unicode grapheme clusters) as defined by Unicode
// Standard Annex #29.
//
// The cursor `pos` is a byte offset that always sits between two grapheme cluster boundaries (or at
// the very start / end of the string).

typedef struct {
    const uint8_t* str;
    size_t str_len;
    size_t pos; // cursor: byte offset, between cluster boundaries
} UcGraphemeIter;

// Initialise the grapheme iterator.
// `pos` must be:
// - 0 for an iterator pointing at the first byte to iterate forward on graphemes
// - str_len for an iterator pointing at the last byte to iterate backward on graphemes
// - a grapheme boundary returned by a previous iteration.
void uc_grapheme_iter_init(UcGraphemeIter* iter, const uint8_t* str, size_t len, size_t pos);

// Advance the cursor to the next cluster boundary.
// Writes the consumed cluster span into *out (may be NULL).
// Returns false when already at the end of the string.
bool uc_grapheme_iter_next(UcGraphemeIter* iter, UcSpan* out);

// Retract the cursor to the previous cluster boundary.
// Writes the consumed cluster span into *out (may be NULL).
// Returns false when already at the start of the string.
bool uc_grapheme_iter_prev(UcGraphemeIter* iter, UcSpan* out);

// Word iterator
//
// Groups consecutive grapheme clusters into word segments by Unicode category:
//   WORD_KIND_WORD  – letters, digits, connector punctuation (e.g. '_')
//   WORD_KIND_SPACE – whitespace and line/paragraph separators
//   WORD_KIND_PUNCT – everything else (punctuation, symbols, control chars)
//
// The cursor `pos` is a byte offset that always sits between two segment boundaries (or at the very
// start / end of the string).

typedef enum {
    UC_WORD_KIND_WORD,
    UC_WORD_KIND_SPACE,
    UC_WORD_KIND_PUNCT,
} UcWordKind;

typedef struct {
    UcSpan span;
    UcWordKind kind;
} UcWordSegment;

typedef struct {
    const uint8_t* str;
    size_t str_len;
    size_t pos; // cursor: byte offset, between segment boundaries
} UcWordIter;

// Initialise the word iterator.
// `pos` must be:
// - 0 for an iterator pointing at the first byte to iterate forward on words
// - str_len for an iterator pointing at the last byte to iterate backward on words
// - a word boundary returned by a previous iteration.
void uc_word_iter_init(UcWordIter* iter, const uint8_t* str, size_t len, size_t pos);

// Advance the cursor to the next segment boundary.
// Writes the consumed segment into *out (may be NULL).
// Returns false when already at the end of the string.
bool uc_word_iter_next(UcWordIter* iter, UcWordSegment* out);

// Retract the cursor to the previous segment boundary.
// Writes the consumed segment into *out (may be NULL).
// Returns false when already at the start of the string.
bool uc_word_iter_prev(UcWordIter* iter, UcWordSegment* out);

#endif // MEDIT_UNICODE_ITER_H_
