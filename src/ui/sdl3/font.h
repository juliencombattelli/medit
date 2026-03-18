#ifndef MEDIT_UI_SDL3_FONT_H_
#define MEDIT_UI_SDL3_FONT_H_

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

TTF_Font* load_emoji_font_aligned_to(TTF_Font* font, const char* path, int size);

#endif // MEDIT_UI_SDL3_FONT_H_
