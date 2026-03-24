#include "unicode.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int g_failures = 0;

#define CHECK_EQ(a, b)                                                                             \
    do {                                                                                           \
        size_t _a = (size_t)(a);                                                                   \
        size_t _b = (size_t)(b);                                                                   \
        if (_a != _b) {                                                                            \
            (void)fprintf(                                                                         \
                stderr,                                                                            \
                "%s:%d: FAIL  %s == %s  (%zu != %zu)\n",                                           \
                __FILE__,                                                                          \
                __LINE__,                                                                          \
                #a,                                                                                \
                #b,                                                                                \
                _a,                                                                                \
                _b);                                                                               \
            ++g_failures;                                                                          \
        }                                                                                          \
    } while (0)

static const char* word_kind_name(UcWordKind k)
{
    switch (k) {
        case UC_WORD_KIND_WORD: return "WORD ";
        case UC_WORD_KIND_SPACE: return "SPACE";
        case UC_WORD_KIND_PUNCT: return "PUNCT";
    }
    return "?????";
}

static void test_grapheme_forward(const char* str, size_t len)
{
    printf("=== grapheme forward ===\n");
    UcGraphemeIter it;
    uc_grapheme_iter_init(&it, (uint8_t*)str, len, 0);

    UcSpan span;
    while (uc_grapheme_iter_next(&it, &span)) {
        printf("  [%3zu +%2zu]  \"", span.start, span.len);
        (void)fwrite(str + span.start, 1, span.len, stdout);
        printf("\"\n");
    }
}

static void test_grapheme_backward(const char* str, size_t len)
{
    printf("=== grapheme backward ===\n");
    UcGraphemeIter it;
    uc_grapheme_iter_init(&it, (uint8_t*)str, len, len);

    UcSpan span;
    while (uc_grapheme_iter_prev(&it, &span)) {
        printf("  [%3zu +%2zu]  \"", span.start, span.len);
        (void)fwrite(str + span.start, 1, span.len, stdout);
        printf("\"\n");
    }
}

// Forward and backward iteration must visit the same spans (in reverse order)
// and produce the same count.
static void test_grapheme_roundtrip(const char* str, size_t len)
{
#define MAX_SPANS 128
    UcSpan fwd[MAX_SPANS];
    UcSpan bwd[MAX_SPANS];
    size_t nf = 0;
    size_t nb = 0;

    UcGraphemeIter it;

    uc_grapheme_iter_init(&it, (uint8_t*)str, len, 0);
    while (nf < MAX_SPANS && uc_grapheme_iter_next(&it, &fwd[nf])) {
        ++nf;
    }

    uc_grapheme_iter_init(&it, (uint8_t*)str, len, len);
    while (nb < MAX_SPANS && uc_grapheme_iter_prev(&it, &bwd[nb])) {
        ++nb;
    }

    CHECK_EQ(nf, nb);
    for (size_t i = 0; i < nf && i < nb; ++i) {
        size_t ri = nf - 1u - i;
        CHECK_EQ(fwd[i].start, bwd[ri].start);
        CHECK_EQ(fwd[i].len, bwd[ri].len);
    }
    printf("grapheme roundtrip: %zu clusters  %s\n", nf, g_failures ? "FAIL" : "OK");
#undef MAX_SPANS
}

// Iterating forward then backward from the midpoint must recover the start.
static void test_grapheme_midpoint(const char* str, size_t len)
{
    // Advance exactly half the clusters from the start.
    UcGraphemeIter it;
    uc_grapheme_iter_init(&it, (uint8_t*)str, len, 0);

    size_t total = 0;
    while (uc_grapheme_iter_next(&it, NULL)) {
        ++total;
    }

    size_t half = total / 2u;
    uc_grapheme_iter_init(&it, (uint8_t*)str, len, 0);
    for (size_t i = 0; i < half; ++i) {
        uc_grapheme_iter_next(&it, NULL);
    }

    size_t mid = it.pos;

    // Now go backward half steps: we must land back at 0.
    uc_grapheme_iter_init(&it, (uint8_t*)str, len, mid);
    for (size_t i = 0; i < half; ++i) {
        uc_grapheme_iter_prev(&it, NULL);
    }

    CHECK_EQ(it.pos, 0u);
    printf(
        "grapheme midpoint:  mid=%zu  recovered=%zu  %s\n",
        mid,
        it.pos,
        g_failures ? "FAIL" : "OK");
}

static void test_word_forward(const char* str, size_t len)
{
    printf("=== word forward ===\n");
    UcWordIter it;
    uc_word_iter_init(&it, (uint8_t*)str, len, 0);

    UcWordSegment seg;
    while (uc_word_iter_next(&it, &seg)) {
        printf("  [%3zu +%2zu] %s  \"", seg.span.start, seg.span.len, word_kind_name(seg.kind));
        (void)fwrite(str + seg.span.start, 1, seg.span.len, stdout);
        printf("\"\n");
    }
}

static void test_word_backward(const char* str, size_t len)
{
    printf("=== word backward ===\n");
    UcWordIter it;
    uc_word_iter_init(&it, (uint8_t*)str, len, len);

    UcWordSegment seg;
    while (uc_word_iter_prev(&it, &seg)) {
        printf("  [%3zu +%2zu] %s  \"", seg.span.start, seg.span.len, word_kind_name(seg.kind));
        (void)fwrite(str + seg.span.start, 1, seg.span.len, stdout);
        printf("\"\n");
    }
}

static void test_word_roundtrip(const char* str, size_t len)
{
#define MAX_SEGS 128
    UcWordSegment fwd[MAX_SEGS];
    UcWordSegment bwd[MAX_SEGS];
    size_t nf = 0;
    size_t nb = 0;

    UcWordIter it;

    uc_word_iter_init(&it, (uint8_t*)str, len, 0);
    while (nf < MAX_SEGS && uc_word_iter_next(&it, &fwd[nf])) {
        ++nf;
    }

    uc_word_iter_init(&it, (uint8_t*)str, len, len);
    while (nb < MAX_SEGS && uc_word_iter_prev(&it, &bwd[nb])) {
        ++nb;
    }

    CHECK_EQ(nf, nb);
    for (size_t i = 0; i < nf && i < nb; ++i) {
        size_t ri = nf - 1u - i;
        CHECK_EQ(fwd[i].span.start, bwd[ri].span.start);
        CHECK_EQ(fwd[i].span.len, bwd[ri].span.len);
        CHECK_EQ((size_t)fwd[i].kind, (size_t)bwd[ri].kind);
    }
    printf("word roundtrip:     %zu segments  %s\n", nf, g_failures ? "FAIL" : "OK");
#undef MAX_SEGS
}

int main(void)
{
    // Mix of ASCII, combining sequences, ZWJ emoji, regional indicators,
    // Devanagari (SpacingMark), and Tamil (SpacingMark).
    const char* unicode_text = "Tëst 👨‍👩‍👦 🇺🇸 नि நி!";
    size_t unicode_len = strlen(unicode_text);

    const char* word_text = "Hello, world! 42 _underscoreनि_test";
    size_t word_len = strlen(word_text);

    printf("\n[unicode string: \"%s\"]\n\n", unicode_text);
    test_grapheme_forward(unicode_text, unicode_len);
    printf("\n");
    test_grapheme_backward(unicode_text, unicode_len);
    printf("\n");
    test_grapheme_roundtrip(unicode_text, unicode_len);
    test_grapheme_midpoint(unicode_text, unicode_len);

    printf("\n[word string: \"%s\"]\n\n", word_text);
    test_word_forward(word_text, word_len);
    printf("\n");
    test_word_backward(word_text, word_len);
    printf("\n");
    test_word_roundtrip(word_text, word_len);

    printf("\n%s\n", g_failures == 0 ? "All tests passed." : "SOME TESTS FAILED.");
    return g_failures != 0;
}
