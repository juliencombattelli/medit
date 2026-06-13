#ifndef LOADER_H_
#define LOADER_H_

#include <stdbool.h>

typedef struct {
    void* app_state;
    int return_code;
    bool should_reload;
} AppResult;

AppResult app_main(void* old_app_state);

#endif // LOADER_H_
