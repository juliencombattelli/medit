#include "sdl3_internal.h"

#include <core/assert.h>
#include <core/safeint.h>
#include <core/utils.h>

static void action_dump_state(Meditor* medit, void* display)
{
    MEDIT_UNUSED(display);
    medit_dump_state(medit);
}

static void action_save_file(Meditor* medit, void* display)
{
    MEDIT_UNUSED(display);
    medit_save_focused_file(medit);
}

static void display_sdl3_open_file_dialog_cb(void* userdata, const char* const* filelist, int filter)
{
    MEDIT_UNUSED(filter);

    SDL3Display* display = userdata;

    if (!filelist) {
        (void)fprintf(stderr, "Error: %s\n", SDL_GetError());
        return;
    }
    if (!*filelist) {
        printf("The user did not select any file.\n");
        return;
    }
    while (*filelist) {
        medit_load_file(display->medit, *filelist);
        filelist++;
    }
}

static void action_open_file_dialog(Meditor* medit, void* display_)
{
    MEDIT_UNUSED(medit);

    SDL3Display* display = display_;
    SDL_ShowOpenFileDialog(display_sdl3_open_file_dialog_cb, display, NULL, NULL, 0, NULL, 1);
}

static void action_toggle_side_panel(Meditor* medit, void* display_)
{
    MEDIT_UNUSED(medit);
    MEDIT_UNUSED(display_);
}

const Actions DISPLAY_SDL3_ACTIONS = {
    MEDIT_CORE_ACTIONS_DEFAULT,
    .save_file = action_save_file,
    .open_file_dialog = action_open_file_dialog,
    .toggle_side_panel = action_toggle_side_panel,
    .dump_state = action_dump_state,
};

static void display_sdl3_handle_save_of_dirty_file(
    SDL3Display* display,
    File* file,
    SDL_MessageBoxData* messageboxdata,
    bool* cancel_exit)
{
    *cancel_exit = false;

    static const char fmt[] = "Do you want to save the changes you made to %s?";
    int format_ret = snprintf(NULL, 0, fmt, file->name) + 1; // +1 for null terminator
    size_t message_len = int_to_size(format_ret);
    char* msg = calloc(message_len, 1);
    (void)snprintf(msg, message_len, fmt, file->name);
    messageboxdata->message = msg;

    if (file->dirty) {
        int buttonid = 0;
        assert(SDL_ShowMessageBox(messageboxdata, &buttonid));
        switch (buttonid) {
            case 0:
                printf("Saving changes for file %s\n", file->name);
                medit_save_focused_file(display->medit);
                break;
            case 1: printf("Discarding changes for file %s\n", file->name); break;
            default: printf("Cancelling exit\n"); *cancel_exit = true;
        }
    }

    free(msg);
}

void display_sdl3_handle_save_of_dirty_files(SDL3Display* display)
{
    static const SDL_MessageBoxButtonData buttons[] = {
        {
            .flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
            .buttonID = 0,
            .text = "Save",
        },
        {
            .flags = 0,
            .buttonID = 1,
            .text = "Don't Save",
        },
        {
            .flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT,
            .buttonID = 2,
            .text = "Cancel",
        },
    };

    static SDL_MessageBoxData messageboxdata = {
        .flags = SDL_MESSAGEBOX_WARNING | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT,
        .window = NULL,
        .title = "Medit",
        .message = NULL,
        .numbuttons = SDL_arraysize(buttons),
        .buttons = buttons,
        .colorScheme = NULL,
    };

    for (size_t i = 0; i < display->medit->opened_files.count; ++i) {
        File* file = &display->medit->opened_files.items[i];
        bool cancel_exit = false;
        display_sdl3_handle_save_of_dirty_file(display, file, &messageboxdata, &cancel_exit);
        if (cancel_exit) {
            break;
        }
    }
}

void display_sdl3_on_text_input(SDL3Display* display, const char* text)
{
    size_t text_len = strlen(text);
    medit_insert_text(display->medit, text, text_len);
    medit_cursor_right(display->medit);
}

void display_sdl3_on_key_down(SDL3Display* display, SDL_Event* event)
{
    switch (event->key.key) {
        case SDLK_RETURN: {
            medit_split_line_at_cursor(display->medit);
            medit_cursor_down(display->medit);
            medit_cursor_line_begin(display->medit);
            medit_file_view_file(display->medit, medit_get_focused_file_view(display->medit))->dirty = true;
        } break;
        case SDLK_BACKSPACE:
            medit_erase_char(display->medit);
            medit_file_view_file(display->medit, medit_get_focused_file_view(display->medit))->dirty = true;
            break;
        default: break;
    }
}
