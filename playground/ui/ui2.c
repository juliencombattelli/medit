#include "ui2.h"
#include <core/dynarray.h>

static void ui2_draw_cmd_list_init(Ui2DrawCmdList* draw_cmd_list)
{
    *draw_cmd_list = (Ui2DrawCmdList) { 0 };
}

static void ui2_draw_cmd_list_free(Ui2DrawCmdList* draw_cmd_list)
{
    dynarray_free(*draw_cmd_list);
}

static void ui2_draw_cmd_list_clear(Ui2DrawCmdList* draw_cmd_list)
{
    draw_cmd_list->count = 0;
}

void ui2_init(Ui2Context* ctx, Ui2Arena arena)
{
    (void)arena;
    ui2_draw_cmd_list_init(&ctx->draw_cmd_list);
}

void ui2_frame_begin(Ui2Context* ctx)
{
}

void ui2_element_begin(Ui2Context* ctx, Ui2ElementDescriptor desc)
{
}

void ui2_element_end(Ui2Context* ctx)
{
}

Ui2DrawCmdList ui2_frame_end(Ui2Context* ctx)
{
    return ctx->draw_cmd_list;
}
