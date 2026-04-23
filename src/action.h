#ifndef MEDIT_ACTION_H_
#define MEDIT_ACTION_H_

typedef struct Meditor Meditor;

typedef struct {
    ////////////////////////////////////////////////////////////////////////////
    /// Core actions, overridble by UI backends
    ////////////////////////////////////////////////////////////////////////////

    // Miscelaneous
    void (*quit)(Meditor* medit, void* ui);

    // Cursor navigation
    void (*cursor_up)(Meditor* medit, void* ui);
    void (*cursor_down)(Meditor* medit, void* ui);
    void (*cursor_left)(Meditor* medit, void* ui);
    void (*cursor_right)(Meditor* medit, void* ui);
    void (*cursor_line_begin)(Meditor* medit, void* ui);
    void (*cursor_line_end)(Meditor* medit, void* ui);
    void (*cursor_file_begin)(Meditor* medit, void* ui);
    void (*cursor_file_end)(Meditor* medit, void* ui);

    // Multi-cursor handling
    void (*add_cursor_down)(Meditor* medit, void* ui);
    void (*restore_cursor)(Meditor* medit, void* ui);

    // Editing
    void (*erase_line)(Meditor* medit, void* ui);

    // Focus control
    void (*focus_file_view_group_left)(Meditor* medit, void* ui);
    void (*focus_file_view_group_right)(Meditor* medit, void* ui);
    void (*display_file_view_left)(Meditor* medit, void* ui);
    void (*display_file_view_right)(Meditor* medit, void* ui);

    // Font zoom
    void (*font_zoom_in)(Meditor* medit, void* ui);
    void (*font_zoom_out)(Meditor* medit, void* ui);
    void (*font_zoom_default)(Meditor* medit, void* ui);

    ////////////////////////////////////////////////////////////////////////////
    /// UI-specific actions, must be provided by UI backends
    ////////////////////////////////////////////////////////////////////////////

    // Dialogs
    void (*save_file)(Meditor* medit, void* ui);
    void (*open_file_dialog)(Meditor* medit, void* ui);

    // Panels control
    void (*toggle_side_panel)(Meditor* medit, void* ui);

    // Debugging
    void (*dump_state)(Meditor* medit, void* ui);
} Actions;

// Default implementation of core actions
void medit_action_quit(Meditor* medit, void* ui);
void medit_action_cursor_up(Meditor* medit, void* ui);
void medit_action_cursor_down(Meditor* medit, void* ui);
void medit_action_cursor_left(Meditor* medit, void* ui);
void medit_action_cursor_right(Meditor* medit, void* ui);
void medit_action_cursor_line_begin(Meditor* medit, void* ui);
void medit_action_cursor_line_end(Meditor* medit, void* ui);
void medit_action_cursor_file_begin(Meditor* medit, void* ui);
void medit_action_cursor_file_end(Meditor* medit, void* ui);
void medit_action_add_cursor_down(Meditor* medit, void* ui);
void medit_action_restore_cursor(Meditor* medit, void* ui);
void medit_action_erase_line(Meditor* medit, void* ui);
void medit_action_focus_file_view_group_left(Meditor* medit, void* ui);
void medit_action_focus_file_view_group_right(Meditor* medit, void* ui);
void medit_action_display_file_view_left(Meditor* medit, void* ui);
void medit_action_display_file_view_right(Meditor* medit, void* ui);
void medit_action_font_zoom_in(Meditor* medit, void* ui);
void medit_action_font_zoom_out(Meditor* medit, void* ui);
void medit_action_font_zoom_default(Meditor* medit, void* ui);

// Helper macros to initialize an Actions struct with default core actions
// clang-format off
#define MEDIT_CORE_ACTIONS_DEFAULT                                                                 \
    .quit = medit_action_quit,                                                                     \
    .cursor_up = medit_action_cursor_up,                                                           \
    .cursor_down = medit_action_cursor_down,                                                       \
    .cursor_left = medit_action_cursor_left,                                                       \
    .cursor_right = medit_action_cursor_right,                                                     \
    .cursor_line_begin = medit_action_cursor_line_begin,                                           \
    .cursor_line_end = medit_action_cursor_line_end,                                               \
    .cursor_file_begin = medit_action_cursor_file_begin,                                           \
    .cursor_file_end = medit_action_cursor_file_end,                                               \
    .add_cursor_down = medit_action_add_cursor_down,                                               \
    .restore_cursor = medit_action_restore_cursor,                                                 \
    .erase_line = medit_action_erase_line,                                                         \
    .focus_file_view_group_left = medit_action_focus_file_view_group_left,                         \
    .focus_file_view_group_right = medit_action_focus_file_view_group_right,                       \
    .display_file_view_left = medit_action_display_file_view_left,                                 \
    .display_file_view_right = medit_action_display_file_view_right,                               \
    .font_zoom_in =   medit_action_font_zoom_in,                                                   \
    .font_zoom_out = medit_action_font_zoom_out,                                                   \
    .font_zoom_default = medit_action_font_zoom_default
// clang-format on

#endif // MEDIT_ACTION_H_
