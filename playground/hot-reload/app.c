#include "app.h"

#include <core/assert.h>

#include <stdio.h>
#include <stdlib.h>

int app_main(AppState* app_state)
{
    if (!app_state->reload_requested) {
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
    } else {
        app_state->reload_requested = false;
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
                        printf("Hot-reloading application...\n");
                        app_state->reload_requested = true;
                        running = false;
                    }
                case SDL_EVENT_TEXT_INPUT:
                case SDL_EVENT_KEYMAP_CHANGED:
                default: break;
            }
        }

        SDL_SetRenderDrawColor(app_state->renderer, 0, 128, 0, 255);
        SDL_RenderClear(app_state->renderer);

        SDL_RenderPresent(app_state->renderer);
    }

    if (app_state->reload_requested) {
        // Exit without cleaning up to allow hot-reloading
        return 0;
    }

    SDL_StopTextInput(app_state->window);

    TTF_DestroyRendererTextEngine(app_state->text_engine);
    SDL_DestroyRenderer(app_state->renderer);
    SDL_DestroyWindow(app_state->window);

    TTF_Quit();
    SDL_Quit();

    return 0;
}
