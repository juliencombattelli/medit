#include "meditor.h"

#include <string.h>

void meditor_cursor_up(Meditor* medit, int cells)
{
    medit->cursor_row -= cells;
    if (medit->cursor_row < 0) {
        medit->cursor_row = 0;
    }
}

void meditor_cursor_down(Meditor* medit, int cells)
{
    medit->cursor_row += cells;
    if (medit->cursor_row >= medit->grid_rows) {
        medit->cursor_row = medit->grid_rows - 1;
    }
}

void meditor_cursor_left(Meditor* medit, int cells)
{
    medit->cursor_col -= cells;
    if (medit->cursor_col < 0) {
        medit->cursor_col = 0;
    }
}

void meditor_cursor_right(Meditor* medit, int cells)
{
    medit->cursor_col += cells;
    if (medit->cursor_col >= medit->grid_cols) {
        medit->cursor_col = medit->grid_cols - 1;
    }
}

void meditor_append_text(Meditor* medit, const char* text)
{
    const size_t input_size = strlen(text);
    const size_t free_space = TEXT_CAPACITY - medit->text_size;
    if (medit->text_size > free_space) {
        medit->text_size = free_space;
    }
    memcpy(medit->text + medit->text_size, text, input_size);
    medit->text_size += input_size;
}
