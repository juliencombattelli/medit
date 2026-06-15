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
    // Windows is locking files that are used by a process. For DLLs, they stay
    // locked even after unloading them, preventing any update of the libraries
    // while the process locking them is still running. To avoid this, before
    // loading we first copy the DLL to a new file suffixed by a unique ID (or
    // generation) that is updated for each reload.
    static uint32_t generation = 0;
#define U32_MAX_DIGIT_COUNT 10
#define APP_LIBRARY_COPY_MAX_LEN (sizeof(app_library) + sizeof('-') + U32_MAX_DIGIT_COUNT)
    char app_library_copy[APP_LIBRARY_COPY_MAX_LEN] = { 0 };
    int len = SDL_snprintf(app_library_copy, sizeof(app_library_copy), "%s-%u", app_library, generation++);
    if (len > 0 && len < (int)sizeof(app_library_copy)) {
        if (!SDL_CopyFile(app_library, app_library_copy)) {
            (void)fprintf(stderr, "%s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
        lib = app_library_copy;
    }
#else
    // When SDL is a shared library, SDL_LoadObject (which wraps dlopen) is
    // called from within libSDL3.so. The dynamic linker then uses SDL's own
    // RUNPATH to search for bare filenames, not the executable's $ORIGIN.
    // Passing a full path makes dlopen open the file directly, bypassing
    // RUNPATH lookup entirely.
    const char* const base_path = SDL_GetBasePath();
    if (!base_path) {
        (void)fprintf(stderr, "%s\n", "Failed to get app base path, attempting to load the app library with relative path instead.");
    } else {
#define APP_LIBRARY_FULL_PATH_MAX_LEN 4096
        char app_library_full_path[APP_LIBRARY_FULL_PATH_MAX_LEN];
        int len = SDL_snprintf(app_library_full_path, sizeof(app_library_full_path), "%s%s", base_path, app_library);
        if (len > 0 && len < (int)sizeof(app_library_full_path)) {
            lib = app_library_full_path;
        }
    }
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
