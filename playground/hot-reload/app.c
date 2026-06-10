#include "loader.h"

#include <core/assert.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct AppState {
    void* dlh;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
} AppState;

int app_main(void* dl_handle, AppState* app_state)
{
    bool hot_reloading = app_state->dlh != NULL;

    if (!hot_reloading) {
        printf("Starting application\n");

        *app_state = (AppState){ 0 };

        assert(SDL_Init(SDL_INIT_VIDEO));
        assert(TTF_Init());

        app_state->window = SDL_CreateWindow("UI Playground", 1280, 720, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
        assert(app_state->window);

        app_state->renderer = SDL_CreateRenderer(app_state->window, NULL);
        assert(app_state->renderer);

        assert(SDL_SetRenderVSync(app_state->renderer, 1));

        app_state->text_engine = TTF_CreateRendererTextEngine(app_state->renderer);
        assert(app_state->text_engine);

        assert(SDL_ShowWindow(app_state->window));
        assert(SDL_StartTextInput(app_state->window));

        printf("Application started\n");
    } else {
        printf("Restoring state\n");
        printf("Application hot-reloaded\n");
    }

    bool running = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_F5) {
                        printf("Hot-reloading application\n");
                        app_state->dlh = dl_handle;
                        running = false;  // Exit event loop to trigger reload
                    }
                case SDL_EVENT_TEXT_INPUT:
                case SDL_EVENT_KEYMAP_CHANGED:
                default: break;
            }
        }

        static size_t cnt = 0;
        printf("Hello %zu\n", cnt++);
        SDL_SetRenderDrawColor(app_state->renderer, 0, 128, 0, 255);
        SDL_RenderClear(app_state->renderer);

        SDL_RenderPresent(app_state->renderer);
    }

    SDL_StopTextInput(app_state->window);

    TTF_DestroyRendererTextEngine(app_state->text_engine);
    SDL_DestroyRenderer(app_state->renderer);
    SDL_DestroyWindow(app_state->window);

    TTF_Quit();
    SDL_Quit();

    int ret = running ? 0 : 1;  // Return 1 if reload was requested
    return ret;
}
