#ifndef APP_H_
#define APP_H_

#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
    bool reload_requested;
} AppState;

int app_main(AppState* app_state);

#endif // APP_H_
