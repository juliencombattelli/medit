#include "loader.h"
#include "app.h"

#include <stdlib.h>
#include <stdio.h>

int main(void) {
    AppState* state = (AppState*)malloc(sizeof(AppState));
    if (!state) {
        fprintf(stderr, "Failed to allocate state\n");
        return 1;
    }
    *state = (AppState){ 0 };

    while (1) {
        int ret = app_reload(state);
        if (ret == 0) break;  // Normal exit (return value 0)
        // If return value is 1, reload continues with same state
    }

    free(state);
    return 0;
}
