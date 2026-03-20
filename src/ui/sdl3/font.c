#include "font.h"

#include <stdio.h>
#include <stdlib.h>

// **Note about emoji**
// A fallback font is used to render emoji. As we want emoji to fit in a grid
// cell, we compute the size of an emoji glyph (currently 😀) so that it is
// equal to the size of a letter glyph in the main editor font. The computation
// is using strings of a hundred glyphs for a better precision.
// This method works well for the currently used fonts and size but it is
// expected to be brittle. Also, rendered emoji are quite small and might not be
// properly readable at all...

#define FONT_TEST_CHAR_COUNT (100)

static const char editor_font_test_string[] = //
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM"
    "MMMMMMMMMM";
_Static_assert(sizeof(editor_font_test_string) - 1 == FONT_TEST_CHAR_COUNT * (sizeof("M") - 1), "");

static const char emoji_font_test_string[] = //
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀"
    "😀😀😀😀😀😀😀😀😀😀";
_Static_assert(sizeof(emoji_font_test_string) - 1 == FONT_TEST_CHAR_COUNT * (sizeof("😀") - 1), "");

static int glyph_width(TTF_Font* font, const char* s)
{
    int w = 0;
    if (!TTF_MeasureString(font, s, 0, 0, &w, NULL)) {
        printf("Error: failed to get font metric: %s\n", SDL_GetError());
        return 0;
    }
    return w / FONT_TEST_CHAR_COUNT;
}

TTF_Font* load_emoji_font_aligned_to(TTF_Font* font, const char* path, int size, int width_factor)
{
    if (width_factor == 0) {
        return TTF_OpenFont(path, (float)size);
    }

    const int main_font_w = glyph_width(font, editor_font_test_string);
    float factor = 1.0f;
#define FORCE_MONOSPACE_FONTS_MAX_ITER 128
    int iter = FORCE_MONOSPACE_FONTS_MAX_ITER;
    while (--iter) {
        TTF_Font* emoji_font = TTF_OpenFont(path, (float)size * factor);
        int emoji_font_w = glyph_width(emoji_font, emoji_font_test_string);
        if (emoji_font_w > width_factor * main_font_w) {
            factor -= factor / 2.0f;
        } else if (emoji_font_w < width_factor * main_font_w) {
            factor += factor / 2.0f;
        } else if (emoji_font_w == width_factor * main_font_w) {
            return emoji_font;
        }
        TTF_CloseFont(emoji_font);
    }
    return NULL;
}
