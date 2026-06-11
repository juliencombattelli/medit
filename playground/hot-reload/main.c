#include "app.h"

#include <dlfcn.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

typedef int AppMainFn(AppState*);

typedef struct {
    int return_code;
    bool should_reload;
} AppResult;

AppResult app_reload(AppState* app_state) {
    static void* dlh = NULL;

    if (dlh) {
        printf("Unloading old lib\n");
        dlclose(dlh);
        dlh = NULL;
    }

    dlh = dlopen("libplayground_hot_reloadable_app.so", RTLD_NOW | RTLD_LOCAL);
    if (!dlh) {
        (void)fprintf(stderr, "%s\n", dlerror());
        (void)exit(EXIT_FAILURE);
    }
    AppMainFn *app_main_fn = (AppMainFn*)dlsym(dlh, "app_main");
    if (!app_main_fn) {
        (void)fprintf(stderr, "%s\n", dlerror());
        (void)exit(EXIT_FAILURE);
    }

    return (AppResult) {
        .return_code = app_main_fn(app_state),
        .should_reload = app_state->reload_requested,
    };
}


int main(void) {
    AppState* state = (AppState*)malloc(sizeof(AppState));
    if (!state) {
        (void)fprintf(stderr, "Failed to allocate state\n");
        return 1;
    }

    int exit_code = 0;
    while (1) {
        AppResult result = app_reload(state);
        if (result.should_reload == false || result.return_code != 0) {
            exit_code = result.return_code;
            break;
        }
    }

    free(state);

    return exit_code;
}
