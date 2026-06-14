#include "loader.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HOT_RELOAD_ENABLE

#include <dlfcn.h>
#include <unistd.h>

// NOLINTBEGIN(*concurrency-mt-unsafe*): the loader is single threaded

typedef AppResult AppMainFn(void*);

static const char* const app_library = "libplayground_hot_reloadable_app.so";

AppResult app_reload(void* old_app_state)
{
    static void* lib_handler = NULL;

    if (lib_handler) {
        printf("Unloading old lib\n");
        dlclose(lib_handler);
        lib_handler = NULL;
    }

    lib_handler = dlopen(app_library, RTLD_NOW | RTLD_LOCAL);
    if (!lib_handler) {
        (void)fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    AppMainFn *app_main_fn = (AppMainFn*)dlsym(lib_handler, "app_main");
#pragma GCC diagnostic pop
    if (!app_main_fn) {
        (void)fprintf(stderr, "%s\n", dlerror());
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
