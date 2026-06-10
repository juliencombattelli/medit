#ifndef APP_LOADER_H_
#define APP_LOADER_H_

typedef struct AppState AppState;

typedef int AppMainFn(void* dl_handle, AppState*);

int app_main(void* dl_handle, AppState* previous_app_state);

int app_reload(AppState* app_state);

#endif // APP_LOADER_H_
