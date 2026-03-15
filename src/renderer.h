#ifndef MEDIT_RENDERER_H_
#define MEDIT_RENDERER_H_

#include "color.h"
#include "linalg.h"

typedef struct Meditor Meditor;

typedef void RendererCreateFn(Meditor* medit);
typedef void RendererLoadFontFn(Meditor* medit);
typedef void RendererUnloadFontFn(Meditor* medit);
typedef int RendererGetTextCellsFn(Meditor* medit, const char* text);
typedef void RendererHandleEventsFn(Meditor* medit);
typedef void RendererClearScreenFn(Meditor* medit, Color color);
typedef void RendererRenderTextFn(Meditor* medit, const char* text, int n, Vec2 cell, Color color);
typedef void RendererRenderText0Fn(Meditor* medit, const char* text, Vec2 cell, Color color);
typedef void RendererRenderCursorFn(Meditor* medit, Color color);
typedef void RendererRenderDebugGridFn(Meditor* medit);
typedef void RendererPresentFn(Meditor* medit);
typedef void RendererDestroyFn(Meditor* medit);

typedef struct {
    RendererCreateFn* create;
    RendererLoadFontFn* load_font;
    RendererUnloadFontFn* unload_font;
    RendererGetTextCellsFn* get_text_cells;
    RendererHandleEventsFn* handle_events;
    RendererClearScreenFn* clear_screen;
    RendererRenderTextFn* render_text;
    RendererRenderText0Fn* render_text0;
    RendererRenderCursorFn* render_cursor;
    RendererRenderDebugGridFn* render_debug_grid;
    RendererPresentFn* present;
    RendererDestroyFn* destroy;
} RendererFns;

typedef struct {
    RendererFns fns;
    void* data;
    const char* name;
} Renderer;

void medit_renderer_create(Meditor* meditor);
void medit_load_font(Meditor* medit);
void medit_unload_font(Meditor* medit);
int medit_get_text_cells(Meditor* medit, const char* text);
void medit_handle_events(Meditor* medit);
void medit_clear_screen(Meditor* medit, Color color);
void medit_render_text(Meditor* medit, const char* text, int n, Vec2 cell, Color color);
void medit_render_text0(Meditor* medit, const char* text, Vec2 cell, Color color);
void medit_render_cursor(Meditor* medit, Color color);
void medit_render_debug_grid(Meditor* medit);
void medit_renderer_present(Meditor* medit);
void medit_renderer_destroy(Meditor* medit);

#endif // MEDIT_RENDERER_H_
