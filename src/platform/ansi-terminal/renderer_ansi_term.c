#include "assert.h"
#include "meditor.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#define ESC "\x1B"
#define CSI ESC "["

typedef struct {
    char screen_buffer[1024];
    size_t len;
    int stdin_flags;
    struct termios stdout_initial_termios;
} RendererANSITerm;

static bool terminal_resized;

void print_n(const void* buffer, size_t size)
{
    // TODO handle error
    if (write(STDOUT_FILENO, buffer, size) < 0) { }
}

void print_cstr(const char* str)
{
    print_n(str, strlen(str));
}

static void screen_buffer_append_char(RendererANSITerm* renderer, char c)
{
    renderer->screen_buffer[renderer->len++] = c;
}

static void screen_buffer_append(RendererANSITerm* renderer, const char* s)
{
    assert(s != NULL);
    while (*s != '\0') {
        screen_buffer_append_char(renderer, *s++);
    }
}

static void send_code(RendererANSITerm* renderer, const char* code)
{
    screen_buffer_append(renderer, code);
}

static void cursor_home(RendererANSITerm* renderer)
{
    send_code(renderer, CSI "H");
}

static void cursor_show(RendererANSITerm* renderer)
{
    send_code(renderer, CSI "?25h");
}

void cursor_hide(RendererANSITerm* renderer)
{
    send_code(renderer, CSI "?25l");
}

typedef enum {
    ERASE_IN_LINE_RIGHT_OF_CURSOR = 0,
    ERASE_IN_LINE_LEFT_OF_CURSOR = 1,
    ERASE_LINE = 2,
} EraseInLineMode;

static void erase_line(RendererANSITerm* renderer, EraseInLineMode mode)
{
    switch (mode) {
        case ERASE_IN_LINE_RIGHT_OF_CURSOR: send_code(renderer, CSI "0K"); return;
        case ERASE_IN_LINE_LEFT_OF_CURSOR: send_code(renderer, CSI "1K"); return;
        case ERASE_LINE: send_code(renderer, CSI "2K"); return;
    }
    assert(false && "Not a valid EraseInLineMode");
}

typedef enum {
    CLEAR_CURSOR_TO_TOP = 0,
    CLEAR_CURSOR_TO_BOTTOM = 1,
    CLEAR_SCREEN = 2,
} ClearMode;

static void clear(RendererANSITerm* renderer, ClearMode mode)
{
    switch (mode) {
        case CLEAR_CURSOR_TO_TOP: send_code(renderer, CSI "0J"); return;
        case CLEAR_CURSOR_TO_BOTTOM: send_code(renderer, CSI "1J"); return;
        case CLEAR_SCREEN: send_code(renderer, CSI "2J"); return;
    }
    assert(false && "Not a valid ClearMode");
}

static void enter_main_screen_buffer(void)
{
    static const char code[] = CSI "?1049l";
    // Send code right away
    print_n(code, sizeof(code) - 1);
}

static void enter_alternate_screen_buffer(void)
{
    static const char code[] = CSI "?1049h";
    // Send code right away
    print_n(code, sizeof(code) - 1);
}

static void write_screen_buffer(RendererANSITerm* renderer)
{
    print_n(renderer->screen_buffer, renderer->len);
    memset(renderer->screen_buffer, 0, sizeof(renderer->screen_buffer));
    renderer->len = 0;
}

static void draw_lines(Meditor* medit, RendererANSITerm* renderer)
{
    // char eob_char = '~';
    // for (size_t row = 0; row < medit->grid_size.row; row++) {
    //     erase_line(renderer, ERASE_IN_LINE_RIGHT_OF_CURSOR);
    //     size_t line_index = row + editor::state.viewport_offset.row;
    //     if (line_index < file->lines.size()) {
    //         auto& line = file->lines[line_index];
    //         editor::SubstrResult rendered_line = editor::rendered_column_substr(
    //             line,
    //             editor::state.viewport_offset.col,
    //             editor::get_screen_cols());
    //         editor::screen_buffer_append_substr(rendered_line);
    //     }
    //     editor::screen_buffer_append("\r\n");
    // }
}

static void disable_raw_mode(RendererANSITerm* renderer)
{
    // Restore the original terminal mode
    if (tcsetattr(STDOUT_FILENO, TCSANOW, &renderer->stdout_initial_termios) != 0) {
        assert(false && "tcsetattr() failed");
    }
}

static void handle_resize(int signal)
{
    (void)signal;
    terminal_resized = true;
}

static void enable_raw_mode(RendererANSITerm* renderer)
{
    // Store the stdin flags so we can more easily toggle `O_NONBLOCK` later on.
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL);
    assert(stdin_flags != -1 && "fcntl(F_GETFL) failed");
    renderer->stdin_flags = stdin_flags;

    // Set state.terminal_resized to true whenever we get SIGWINCH
    struct sigaction action = { 0 };
    action.sa_handler = handle_resize;
    if (sigaction(SIGWINCH, &action, NULL) != 0) {
        assert(false && "sigaction(SIGWINCH) failed");
    }

    // Get the original termios
    struct termios raw = { 0 };
    if (tcgetattr(STDOUT_FILENO, &raw) != 0) {
        assert(false && "tcgetattr() failed");
    }
    renderer->stdout_initial_termios = raw;
    // clang-tidy: the `0U |` trick is used to silence *signed-bitwise*
    raw.c_iflag &= ~(
        0U //
        | BRKINT // Do not signal interrupt on break
        | INPCK // Disable input parity checking
        | ISTRIP // Disable stripping of eighth bit
        | INLCR // Disable mapping of NL to CR on input
        | IGNCR // Disable ignoring CR on input
        | ICRNL // Disable mapping of CR to NL on input
        | IXON // Disable software flow control
    );
    raw.c_oflag &= ~(
        0U //
        | OPOST // Disable output processing
    );
    raw.c_cflag &= ~(
        0U //
        | CSIZE // Reset character size mask
        | PARENB // Disable parity generation
    );
    // Set character size back to 8 bits
    raw.c_cflag |= (unsigned)(CS8);
    raw.c_lflag &= ~(
        0U //
        | ISIG // Disable signal generation (SIGINT, SIGTSTP, SIGQUIT)
        | ICANON // Disable canonical mode (line buffering)
        | ECHO // Disable echoing of input characters
        | ECHONL // Disable echoing of NL
        | IEXTEN // Disable extended input processing (e.g. Ctrl-V)
    );

    // Set the terminal to raw mode
    if (tcsetattr(STDOUT_FILENO, TCSANOW, &raw) != 0) {
        assert(false && "tcsetattr() failed");
    }
}

void init(RendererANSITerm* renderer)
{
    enter_alternate_screen_buffer();
    enable_raw_mode(renderer);

    // atexit(deinit);
}

void deinit(RendererANSITerm* renderer)
{
    disable_raw_mode(renderer);
    enter_main_screen_buffer();
}

static void ansi_renderer_create(Meditor* medit)
{
    RendererANSITerm* renderer = (RendererANSITerm*)medit->renderer.data;

    init(renderer);
    medit_load_default_tui_keybind(medit);
}

static void ansi_render_load_font(Meditor* medit)
{
    // No font control through escape sequences inside an ANSI terminal
    (void)medit;
}

static void ansi_render_unload_font(Meditor* medit)
{
    // No font control through escape sequences inside an ANSI terminal
    (void)medit;
}

static int ansi_get_text_cells(Meditor* medit, const char* text)
{
    // TODO handle unicode
    size_t cells = strlen(text);
    assert(cells <= INT_MAX);
    return (int)cells;
}

static void ansi_clear_screen(Meditor* medit, Color color)
{
    (void)medit, (void)color;
}

static void ansi_handle_events(Meditor* medit)
{
    medit->input_in_frame = true;
}

static void ansi_render_text0(Meditor* medit, const char* text, Cell cell, Color color)
{
    const char s[1024] = { 0 };
    int n = sprintf(s, CSI "%d;%df", cell.row, cell.col);
    print_n(s, n);
}

static void ansi_render_cursor(Meditor* medit, Color color)
{
    // Cursor is renderer by the terminal emulator
    (void)medit, (void)color;
}

static void ansi_render_debug_grid(Meditor* medit)
{
    // Not really feasible from an ANSI terminal
    (void)medit;
}

static void ansi_present(Meditor* medit)
{
    RendererANSITerm* renderer = (RendererANSITerm*)medit->renderer.data;

    cursor_hide(renderer);
    cursor_home(renderer);

    draw_lines(medit, renderer);

    cursor_show(renderer);

    FILE* logfile = fopen("medit.log", "w+");
    fputs(renderer->screen_buffer, logfile);
    fflush(logfile);
    fclose(logfile);

    write_screen_buffer(renderer);
}

static void ansi_destroy(Meditor* medit)
{
    RendererANSITerm* renderer = (RendererANSITerm*)medit->renderer.data;
    deinit(renderer);
    free(renderer);
}

Renderer renderer_ansi_term(void)
{
    return (Renderer) {
        .data = calloc(1, sizeof(RendererANSITerm)),
        .fns = {
            .create = ansi_renderer_create,
            .load_font = ansi_render_load_font,
            .unload_font = ansi_render_unload_font,
            .get_text_cells = ansi_get_text_cells,
            .handle_events = ansi_handle_events,
            .clear_screen = ansi_clear_screen,
            .render_text0 = ansi_render_text0,
            .render_cursor = ansi_render_cursor,
            .render_debug_grid = ansi_render_debug_grid,
            .present = ansi_present,
            .destroy = ansi_destroy,
        },
        .name = "ANSI Terminal",
    };
}
