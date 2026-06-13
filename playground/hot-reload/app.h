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
    size_t frame_count;
} AppState;

typedef struct {
    int return_code;
    bool should_reload;
} AppResult;

AppResult app_main(AppState* app_state, bool reload_requested);

#endif // APP_H_
