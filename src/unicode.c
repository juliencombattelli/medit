#include "unicode.h"

#include "assert.h"

#include <utf8proc.h>

// Maximum codepoints to scan backward when searching for a safe grapheme-cluster anchor.
// 64 is sufficient for every real-world cluster, including country-subdivision flag tag sequences
// (~15 codepoints) and long ZWJ family emoji (~10 codepoints).
#define GRAPHEME_LOOKBACK_LIMIT 64

#define UTF8_CONT_MASK 0xC0u
#define UTF8_CONT_TAG 0x80u

// Move backward one UTF-8 codepoint from byte offset `pos`.
// Returns the byte offset of the start of that codepoint.
// `pos` must be > 0.
static size_t utf8_prev(const uint8_t* str, size_t pos)
{
    assert(pos > 0);
    size_t p = pos - 1u;
    while (p > 0u && (str[p] & UTF8_CONT_MASK) == UTF8_CONT_TAG) {
        --p;
    }
    return p;
}

// Return the byte offset of the start of the grapheme cluster whose end is `end_pos` (i.e. the
// cluster occupying [result, end_pos)).
//
// Strategy:
//   1. Scan backward up to GRAPHEME_LOOKBACK_LIMIT codepoints.  Stop early when we hit a codepoint
//      with boundclass CR / LF / Control, because those always open a new cluster.
//   2. From that anchor, replay the UAX-29 state machine forward to find the last cluster boundary
//      strictly before end_pos.
static size_t grapheme_cluster_find_start(const uint8_t* str, size_t str_len, size_t end_pos)
{
    assert(end_pos > 0u && end_pos <= str_len);

    // --- backward pass: locate a safe anchor ---
    size_t pos = end_pos;
    for (int n = 0; n < GRAPHEME_LOOKBACK_LIMIT && pos > 0u; ++n) {
        pos = utf8_prev(str, pos);

        utf8proc_int32_t cp = 0;
        utf8proc_ssize_t adv = utf8proc_iterate(
            (const utf8proc_uint8_t*)str + pos,
            (utf8proc_ssize_t)(str_len - pos),
            &cp);
        if (adv > 0) {
            const utf8proc_property_t* prop = utf8proc_get_property(cp);
            if (prop->boundclass == UTF8PROC_BOUNDCLASS_CR
                || prop->boundclass == UTF8PROC_BOUNDCLASS_LF
                || prop->boundclass == UTF8PROC_BOUNDCLASS_CONTROL) {
                break; // definite cluster start: use as anchor
            }
        }
    }
    size_t anchor = pos;

    // --- forward pass: replay break state machine from anchor to end_pos ---
    utf8proc_int32_t state = 0;
    utf8proc_int32_t prev_cp = -1;
    size_t cluster_start = anchor;
    size_t i = anchor;

    while (i < end_pos) {
        utf8proc_int32_t cp = 0;
        utf8proc_ssize_t adv = utf8proc_iterate(
            (const utf8proc_uint8_t*)str + i,
            (utf8proc_ssize_t)(str_len - i),
            &cp);
        if (adv <= 0) {
            // Invalid byte: treat as its own cluster.
            cluster_start = i + 1u;
            prev_cp = -1;
            state = 0;
            ++i;
            continue;
        }

        if (prev_cp >= 0 && utf8proc_grapheme_break_stateful(prev_cp, cp, &state)) {
            cluster_start = i;
        }

        prev_cp = cp;
        i += (size_t)adv;
    }

    return cluster_start;
}

// Map a codepoint to its word-segment category.
static UcWordKind codepoint_word_kind(utf8proc_int32_t cp)
{
    if (cp < 0) {
        return UC_WORD_KIND_PUNCT;
    }

    switch (utf8proc_category(cp)) {
        // Letters (all subcategories)
        case UTF8PROC_CATEGORY_LU:
        case UTF8PROC_CATEGORY_LL:
        case UTF8PROC_CATEGORY_LT:
        case UTF8PROC_CATEGORY_LM:
        case UTF8PROC_CATEGORY_LO:
        // Numbers
        case UTF8PROC_CATEGORY_ND:
        case UTF8PROC_CATEGORY_NL:
        case UTF8PROC_CATEGORY_NO:
        // Connector punctuation (underscore, fullwidth underscore, …)
        case UTF8PROC_CATEGORY_PC: return UC_WORD_KIND_WORD;

        // Space / line / paragraph separators
        case UTF8PROC_CATEGORY_ZS:
        case UTF8PROC_CATEGORY_ZL:
        case UTF8PROC_CATEGORY_ZP:
        // Control characters (tab, CR, LF, …)
        case UTF8PROC_CATEGORY_CC: return UC_WORD_KIND_SPACE;

        default: return UC_WORD_KIND_PUNCT;
    }
}

// Return the word-segment category for the grapheme cluster starting at byte offset `start` in
// `str[0..str_len)`.
static UcWordKind span_word_kind(const uint8_t* str, size_t start, size_t str_len)
{
    utf8proc_int32_t cp = 0;
    utf8proc_ssize_t adv = utf8proc_iterate(
        (const utf8proc_uint8_t*)str + start,
        (utf8proc_ssize_t)(str_len - start),
        &cp);
    return codepoint_word_kind(adv > 0 ? cp : -1);
}

void uc_grapheme_iter_init(UcGraphemeIter* iter, const uint8_t* str, size_t len, size_t pos)
{
    assert(pos <= len);
    iter->str = str;
    iter->str_len = len;
    iter->pos = pos;
}

bool uc_grapheme_iter_next(UcGraphemeIter* iter, UcSpan* out)
{
    if (iter->pos >= iter->str_len) {
        return false;
    }

    size_t start = iter->pos;
    utf8proc_int32_t state = 0;
    utf8proc_int32_t prev_cp = -1;
    size_t i = start;

    while (i < iter->str_len) {
        utf8proc_int32_t cp = 0;
        utf8proc_ssize_t adv = utf8proc_iterate(
            (const utf8proc_uint8_t*)iter->str + i,
            (utf8proc_ssize_t)(iter->str_len - i),
            &cp);

        if (adv <= 0) {
            // Invalid byte.  If we already accumulated a valid codepoint, close the current cluster
            // and leave the invalid byte for the next call.  Otherwise consume just this byte as
            // its own cluster.
            if (prev_cp >= 0) {
                break;
            }
            ++i;
            break;
        }

        if (prev_cp >= 0 && utf8proc_grapheme_break_stateful(prev_cp, cp, &state)) {
            break; // cluster boundary lies before cp
        }

        prev_cp = cp;
        i += (size_t)adv;
    }

    if (out) {
        out->start = start;
        out->len = i - start;
    }
    iter->pos = i;
    return true;
}

bool uc_grapheme_iter_prev(UcGraphemeIter* iter, UcSpan* out)
{
    if (iter->pos == 0u) {
        return false;
    }

    size_t end = iter->pos;
    size_t start = grapheme_cluster_find_start(iter->str, iter->str_len, end);

    if (out) {
        out->start = start;
        out->len = end - start;
    }
    iter->pos = start;
    return true;
}

void uc_word_iter_init(UcWordIter* iter, const uint8_t* str, size_t len, size_t pos)
{
    assert(pos <= len);
    iter->str = str;
    iter->str_len = len;
    iter->pos = pos;
}

bool uc_word_iter_next(UcWordIter* iter, UcWordSegment* out)
{
    if (iter->pos >= iter->str_len) {
        return false;
    }

    // Consume the first grapheme cluster to establish the segment kind.
    UcGraphemeIter g;
    uc_grapheme_iter_init(&g, iter->str, iter->str_len, iter->pos);

    UcSpan first;
    if (!uc_grapheme_iter_next(&g, &first)) {
        return false;
    }

    UcWordKind kind = span_word_kind(iter->str, first.start, iter->str_len);
    size_t start = first.start;

    // Keep consuming clusters of the same kind.
    for (;;) {
        size_t saved = g.pos;
        UcSpan span;
        if (!uc_grapheme_iter_next(&g, &span)) {
            break;
        }
        if (span_word_kind(iter->str, span.start, iter->str_len) != kind) {
            g.pos = saved; // put the cluster back
            break;
        }
    }

    if (out) {
        out->span.start = start;
        out->span.len = g.pos - start;
        out->kind = kind;
    }
    iter->pos = g.pos;
    return true;
}

bool uc_word_iter_prev(UcWordIter* iter, UcWordSegment* out)
{
    if (iter->pos == 0u) {
        return false;
    }

    // Consume the last grapheme cluster to establish the segment kind.
    UcGraphemeIter g;
    uc_grapheme_iter_init(&g, iter->str, iter->str_len, iter->pos);

    UcSpan last;
    if (!uc_grapheme_iter_prev(&g, &last)) {
        return false;
    }

    UcWordKind kind = span_word_kind(iter->str, last.start, iter->str_len);
    size_t end = iter->pos;

    // Keep consuming clusters of the same kind (going backward).
    for (;;) {
        size_t saved = g.pos;
        UcSpan span;
        if (!uc_grapheme_iter_prev(&g, &span)) {
            break;
        }
        if (span_word_kind(iter->str, span.start, iter->str_len) != kind) {
            g.pos = saved; // put the cluster back
            break;
        }
    }

    if (out) {
        out->span.start = g.pos;
        out->span.len = end - g.pos;
        out->kind = kind;
    }
    iter->pos = g.pos;
    return true;
}
