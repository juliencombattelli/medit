#include "app.h"

#include <dlfcn.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

// NOLINTBEGIN(*concurrency-mt-unsafe*): the loader is single threaded

typedef AppResult AppMainFn(AppState*, bool);

static const char* const app_library = "libplayground_hot_reloadable_app.so";

AppResult app_reload(AppState* app_state, bool reload_requested)
{
    static void* dlh = NULL;

    if (dlh) {
        printf("Unloading old lib\n");
        dlclose(dlh);
        dlh = NULL;
    }

    dlh = dlopen(app_library, RTLD_NOW | RTLD_LOCAL);
    if (!dlh) {
        (void)fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    AppMainFn *app_main_fn = (AppMainFn*)dlsym(dlh, "app_main");
#pragma GCC diagnostic pop
    if (!app_main_fn) {
        (void)fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    return app_main_fn(app_state, reload_requested);
}

int main(void)
{
    AppState state = { 0 };

    int exit_code = 0;
    bool reload_requested = false;
    while (1) {
        AppResult result = app_reload(&state, reload_requested);
        reload_requested = result.should_reload;

        if (!reload_requested) {
            exit_code = result.return_code;
            break;
        }
    }

    return exit_code;
}

// NOLINTEND(*concurrency-mt-unsafe*)
