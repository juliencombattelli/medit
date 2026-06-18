#include "loader/loader_app_main.h"

#include <core/assert.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
    size_t frame_count;
} AppState;

MeditAppResult MEDIT_LOADER_APP_MAIN_FN(int argc, char** argv, void* old_app_state)
{
    (void)argc, (void)argv;

    bool reload_requested = old_app_state != NULL;
    AppState* app_state = (AppState*)old_app_state;

    if (!reload_requested) {
        app_state = (AppState*)calloc(1, sizeof(AppState));

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
        reload_requested = false;
        printf("Application hot-reloaded\n");
    }

    bool running = true;
    while (running) {
        SDL_Event event = { 0 };
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: running = false; break;
                case SDL_EVENT_KEY_DOWN:
#ifdef MEDIT_HOT_RELOAD_ENABLED
                    if (event.key.key == SDLK_F5) {
                        reload_requested = true;
                        running = false;
                        // Finish the current frame before hot-reloading to
                        // ensure that the app is in a consistent state
                    }
#endif
                case SDL_EVENT_TEXT_INPUT:
                case SDL_EVENT_KEYMAP_CHANGED:
                default: break;
            }
        }

        static size_t local_frame_count = 0;
        printf("Frame count: from_state=%zu, from_static=%zu\n", app_state->frame_count++, local_frame_count++);

        SDL_SetRenderDrawColor(app_state->renderer, 0x18, 0x18, 0x18, 255);
        SDL_RenderClear(app_state->renderer);

        SDL_RenderPresent(app_state->renderer);
    }

    if (reload_requested) {
        // Exit without cleaning up to allow hot-reloading
        printf("Hot-reloading application...\n");
        return (MeditAppResult) { .app_state = app_state, .should_reload = true };
    }

    SDL_StopTextInput(app_state->window);

    TTF_DestroyRendererTextEngine(app_state->text_engine);
    SDL_DestroyRenderer(app_state->renderer);
    SDL_DestroyWindow(app_state->window);

    TTF_Quit();
    SDL_Quit();

    free(app_state);

    return (MeditAppResult) { .return_code = 0 };
}
