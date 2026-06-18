#ifndef MEDIT_LOADER_APP_MAIN_H_
#define MEDIT_LOADER_APP_MAIN_H_

#include <stdbool.h>

#define MEDIT_LOADER_APP_MAIN_FN medit_loader_app_main

typedef struct {
    void* app_state;
    int return_code;
    bool should_reload;
} MeditAppResult;

MeditAppResult MEDIT_LOADER_APP_MAIN_FN(int argc, char** argv, void* old_app_state);

#endif // MEDIT_LOADER_APP_MAIN_H_
