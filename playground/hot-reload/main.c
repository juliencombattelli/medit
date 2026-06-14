#include "loader.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HOT_RELOAD_ENABLE

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_stdinc.h>

// NOLINTBEGIN(*concurrency-mt-unsafe*): the loader is single threaded

typedef AppResult AppMainFn(void*);

static const char app_library[] =
#if SDL_PLATFORM_WINDOWS == 1
    "libplayground_hot_reloadable_app.dll"
#else
    "libplayground_hot_reloadable_app.so"
#endif
;

AppResult app_reload(void* old_app_state)
{
    static SDL_SharedObject* lib_handler = NULL;

    if (lib_handler) {
        printf("Unloading old lib\n");
        SDL_UnloadObject(lib_handler);
        lib_handler = NULL;
    }

    const char* lib = app_library;

#if SDL_PLATFORM_WINDOWS == 1
    static uint32_t generation = 0;
    char app_library_copy[sizeof(app_library) + 12] = { 0 };
    SDL_snprintf(app_library_copy, sizeof(app_library_copy), "%s-%d", app_library, generation++);
    if (!SDL_CopyFile(app_library, app_library_copy)) {
        (void)fprintf(stderr, "%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    lib = app_library_copy;
#endif
    printf("Loading %s\n", lib);
    lib_handler = SDL_LoadObject(lib);
    if (!lib_handler) {
        (void)fprintf(stderr, "%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    AppMainFn *app_main_fn = (AppMainFn*)SDL_LoadFunction(lib_handler, "app_main");
    if (!app_main_fn) {
        (void)fprintf(stderr, "%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    return app_main_fn(old_app_state);
}

int main(void)
{
    void* app_state = NULL;

    int exit_code = 0;
    bool reload_requested = false;
    while (1) {
        AppResult result = app_reload(app_state);
        app_state = result.app_state;
        reload_requested = result.should_reload;

        if (!reload_requested) {
            exit_code = result.return_code;
            break;
        }
    }

    return exit_code;
}

// NOLINTEND(*concurrency-mt-unsafe*)

#else
int main(void)
{
    AppResult result = app_main(NULL);
    return result.return_code;
}
#endif
