#include <loader/loader_app_main.h>

#include <core/meditor.h>
#include <core/utils.h>

#include <ui/sdl3/sdl3.h>

MeditAppResult MEDIT_LOADER_APP_MAIN_FN(int argc, char** argv, void* old_app_state)
{
    MEDIT_UNUSED(argc), MEDIT_UNUSED(argv);

    MeditAppResult medit_app_result = medit_ui_sdl3_run(old_app_state);

    if (medit_app_result.return_code == MEDIT_STATUS_FAILED_TO_CREATE_GUI) {
        printf("Failed to create GUI. Attempting to create a TUI instead.\n");
        MEDIT_TODO("TUI is not implemented yet");
    }

    return medit_app_result;
}
