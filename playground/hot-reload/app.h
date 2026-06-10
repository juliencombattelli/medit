#ifndef APP_H_
#define APP_H_

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct AppState {
    void* dlh;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
} AppState;

#endif // APP_H_
