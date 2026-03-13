#include "renderer.h"
#include "color.h"
#include "meditor.h"

#include <stdio.h>

void medit_load_font(Meditor* medit)
{
    Renderer* renderer = &medit->renderer;
    RendererLoadFontFn* load_font = renderer->fns.load_font;
    if (!load_font) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    load_font(medit);
}

void medit_unload_font(Meditor* medit)
{
    Renderer* renderer = &medit->renderer;
    RendererUnloadFontFn* unload_font = renderer->fns.unload_font;
    if (!unload_font) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    unload_font(medit);
}

int medit_get_text_cells(Meditor* medit, const char* text)
{
    Renderer* renderer = &medit->renderer;
    RendererGetTextCellsFn* get_text_cells = renderer->fns.get_text_cells;
    if (!get_text_cells) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return 0;
    }
    return get_text_cells(medit, text);
}

void medit_clear_screen(Meditor* medit, Color color)
{
    Renderer* renderer = &medit->renderer;
    RendererClearScreenFn* clear_screen = renderer->fns.clear_screen;
    if (!clear_screen) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    clear_screen(medit, color);
}

void medit_render_text0(Meditor* medit, const char* text, int cell_x, int cell_y, Color color)
{
    Renderer* renderer = &medit->renderer;
    RendererRenderText0Fn* render_text0 = renderer->fns.render_text0;
    if (!render_text0) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    render_text0(medit, text, cell_x, cell_y, color);
}

void medit_render_cursor(Meditor* medit, Color color)
{
    Renderer* renderer = &medit->renderer;
    RendererRenderCursorFn* render_cursor = renderer->fns.render_cursor;
    if (!render_cursor) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    render_cursor(medit, color);
}

void medit_render_debug_grid(Meditor* medit)
{
    Renderer* renderer = &medit->renderer;
    RendererRenderDebugGridFn* render_debug_grid = renderer->fns.render_debug_grid;
    if (!render_debug_grid) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    render_debug_grid(medit);
}

void medit_renderer_present(Meditor* medit)
{
    Renderer* renderer = &medit->renderer;
    RendererPresentFn* present = renderer->fns.present;
    if (!present) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    present(medit);
}

void medit_renderer_destroy(Meditor* medit)
{
    Renderer* renderer = &medit->renderer;
    RendererDestroyFn* destroy = renderer->fns.destroy;
    if (!destroy) {
        printf("WARNING: Unimplemented: %s for renderer %s\n", __func__, renderer->name);
        return;
    }
    destroy(medit);
}
