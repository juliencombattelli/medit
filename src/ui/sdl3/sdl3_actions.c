#include "sdl3_internal.h"

#include <core/assert.h>
#include <core/safeint.h>
#include <core/utils.h>

static void action_dump_state(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_dump_state(medit);
}

static void action_save_file(Meditor* medit, void* ui)
{
    MEDIT_UNUSED(ui);
    medit_save_focused_file(medit);
}

static void ui_sdl3_open_file_dialog_cb(void* userdata, const char* const* filelist, int filter)
{
    MEDIT_UNUSED(filter);

    SDL3Ui* ui = userdata;

    if (!filelist) {
        (void)fprintf(stderr, "Error: %s\n", SDL_GetError());
        return;
    }
    if (!*filelist) {
        printf("The user did not select any file.\n");
        return;
    }
    while (*filelist) {
        medit_load_file(ui->medit, *filelist);
        filelist++;
    }
}

static void action_open_file_dialog(Meditor* medit, void* ui_)
{
    MEDIT_UNUSED(medit);

    SDL3Ui* ui = ui_;
    SDL_ShowOpenFileDialog(ui_sdl3_open_file_dialog_cb, ui, NULL, NULL, 0, NULL, 1);
}

static void action_toggle_side_panel(Meditor* medit, void* ui_)
{
    MEDIT_UNUSED(medit);

    SDL3Ui* ui = ui_;
    medit_layout_toggle_shown_element(&ui->layout, LAYOUT_SIDE_PANEL);
    ui_sdl3_recompute_layout(ui);
}

const Actions UI_SDL3_ACTIONS = {
    MEDIT_CORE_ACTIONS_DEFAULT,
    .save_file = action_save_file,
    .open_file_dialog = action_open_file_dialog,
    .toggle_side_panel = action_toggle_side_panel,
    .dump_state = action_dump_state,
};

const LayoutSizes UI_SDL3_DEFAULT_LAYOUT_SIZES = {
    .menu_bar_height = 30,
    .tab_bar_height = 35,
    .side_panel_width = 200,
    .bottom_panel_height = 200,
    .status_bar_height = 30,
    .separator_size = 1,
};

static void ui_sdl3_handle_save_of_dirty_file(
    SDL3Ui* ui,
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
                medit_save_focused_file(ui->medit);
                break;
            case 1: printf("Discarding changes for file %s\n", file->name); break;
            default: printf("Cancelling exit\n"); *cancel_exit = true;
        }
    }

    free(msg);
}

void ui_sdl3_handle_save_of_dirty_files(SDL3Ui* ui)
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

    for (size_t i = 0; i < ui->medit->opened_files.count; ++i) {
        File* file = &ui->medit->opened_files.items[i];
        bool cancel_exit = false;
        ui_sdl3_handle_save_of_dirty_file(ui, file, &messageboxdata, &cancel_exit);
        if (cancel_exit) {
            break;
        }
        ui->medit->running = false;
    }
}

void ui_sdl3_on_text_input(SDL3Ui* ui, const char* text)
{
    size_t text_len = strlen(text);
    medit_insert_text(ui->medit, text, text_len);
    medit_cursor_right(ui->medit);
}

void ui_sdl3_on_key_down(SDL3Ui* ui, SDL_Event* event)
{
    switch (event->key.key) {
        case SDLK_RETURN: {
            medit_split_line_at_cursor(ui->medit);
            medit_cursor_down(ui->medit);
            medit_cursor_line_begin(ui->medit);
            medit_file_view_file(ui->medit, medit_get_focused_file_view(ui->medit))->dirty = true;
        } break;
        case SDLK_BACKSPACE:
            medit_erase_char(ui->medit);
            medit_file_view_file(ui->medit, medit_get_focused_file_view(ui->medit))->dirty = true;
            break;
        default: break;
    }
}
