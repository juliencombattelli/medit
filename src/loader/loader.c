#include "loader.h"

#include <stdlib.h>

#ifdef MEDIT_HOT_RELOAD_ENABLED

#include <stdio.h>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_stdinc.h>

#define MEDIT_LOADER_APP_MAIN_SYM loader_xstr(MEDIT_LOADER_APP_MAIN_FN)
#define loader_xstr(s) loader_str(s)
#define loader_str(s) #s

typedef MeditAppResult MeditAppMainFn(int argc, char** argv, void*);

static const char app_library[] = MEDIT_LOADER_APP_LIBRARY_NAME;

MeditAppResult medit_loader_reload_app(int argc, char** argv, void* old_app_state)
{
    // NOLINTBEGIN(*concurrency-mt-unsafe*): the loader is single threaded

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
#define APP_LIBRARY_COPY_MAX_LEN (sizeof(app_library) + sizeof('.') + U32_MAX_DIGIT_COUNT)
    char app_library_copy[APP_LIBRARY_COPY_MAX_LEN] = { 0 };
    int len = SDL_snprintf(app_library_copy, sizeof(app_library_copy), "%s.%u", app_library, generation++);
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

    MeditAppMainFn *app_main_fn = (MeditAppMainFn*)SDL_LoadFunction(lib_handler, MEDIT_LOADER_APP_MAIN_SYM);
    if (!app_main_fn) {
        (void)fprintf(stderr, "%s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // NOLINTEND(*concurrency-mt-unsafe*)

    return app_main_fn(argc, argv, old_app_state);
}

#endif

int medit_loader_main(int argc, char** argv)
{
#ifdef MEDIT_HOT_RELOAD_ENABLED
    void* app_state = NULL;
    int exit_code = 0;
    bool reload_requested = false;

    while (1) {
        MeditAppResult result = medit_loader_reload_app(argc, argv, app_state);
        app_state = result.app_state;
        reload_requested = result.should_reload;

        if (!reload_requested) {
            exit_code = result.return_code;
            break;
        }
    }

    return exit_code;
#else
    MeditAppResult result = medit_loader_app_main(argc, argv, NULL);
    return result.return_code;
#endif
}
