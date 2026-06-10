#include "loader.h"
#include "app.h"

#include <dlfcn.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

int app_reload(AppState* app_state) {
    void* old_dlh = (app_state && app_state->dlh) ? app_state->dlh : NULL;

    void* dlh = dlopen("libplayground_hot_reloadable_app.so", RTLD_NOW | RTLD_GLOBAL);
    if (!dlh) {
        (void)fprintf(stderr, "%s\n", dlerror());
        (void)exit(EXIT_FAILURE);
    }
    AppMainFn *app_main_fn = (AppMainFn*)dlsym(dlh, "app_main");
    if (!app_main_fn) {
        (void)fprintf(stderr, "%s\n", dlerror());
        (void)exit(EXIT_FAILURE);
    }

    int ret = app_main_fn(dlh, app_state);

    // If app_main returned 1 (reload requested), unload old library NOW
    // (app_main has fully exited, so it's safe to unload)
    if (ret == 1 && old_dlh) {
        printf("Unloading old lib\n");
        dlclose(old_dlh);
    }

    return ret;
}
