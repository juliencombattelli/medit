#include "sdl3_internal.h"

#include "utils/utils.h"

#include <core/utils.h>

#include <string.h>

// Allocate a null-terminated string copy from the per-frame arena
// Returns NULL if the arena is full (the label is silently dropped)
// TODO reallocate a new arena if full? or use buckets?
const char* ui_sdl3_arena_str(SDL3Ui* ui, const char* str, size_t len)
{
    size_t needed = len + 1;
    if (ui->ui_text_arena_used + needed > UI_TEXT_ARENA_SIZE) {
        (void)fprintf(stderr, "ui_text_arena full: dropping label\n");
        return NULL;
    }
    char* dst = &ui->ui_text_arena[ui->ui_text_arena_used];
    memcpy(dst, str, len);
    dst[len] = '\0';
    ui->ui_text_arena_used += needed;
    return dst;
}

void ui_sdl3_draw_frame_begin(SDL3Ui* ui)
{
    ui_draw_cmd_list_clear(&ui->ui_draw_list_bg);
    ui_draw_cmd_list_clear(&ui->ui_draw_list_overlay);
    ui->ui_text_arena_used = 0;

    float m_x = 0.f;
    float m_y = 0.f;
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(&m_x, &m_y);
    bool is_down = (buttons & SDL_BUTTON_LMASK) != 0;
    // float_to_i32 is not absolutely needed here, but it doesn't hurt
    int32_t mouse_x = float_to_i32(medit_clampf(m_x, 0.f, (float)INT32_MAX));
    int32_t mouse_y = float_to_i32(medit_clampf(m_y, 0.f, (float)INT32_MAX));

    ui->ui_ctx_bg = (UiCtx) {
        .draw_list    = &ui->ui_draw_list_bg,
        .scroll_speed = (float)ui->font_editor.line_spacing,
        .input = {
            .x            = mouse_x,
            .y            = mouse_y,
            .left_down    = is_down,
            .left_clicked = ui->ui_mouse_was_down && !is_down,
            .scroll_x     = ui->ui_scroll_delta_x,
            .scroll_y     = ui->ui_scroll_delta_y,
            .scroll_valid = ui->ui_scroll_valid,
        },
    };
    ui->ui_ctx_overlay = (UiCtx) {
        .draw_list    = &ui->ui_draw_list_overlay,
        .scroll_speed = (float)ui->font_editor.line_spacing,
        .input = {
            .x            = mouse_x,
            .y            = mouse_y,
            .left_down    = is_down,
            .left_clicked = ui->ui_mouse_was_down && !is_down,
            .scroll_x     = ui->ui_scroll_delta_x,
            .scroll_y     = ui->ui_scroll_delta_y,
            .scroll_valid = ui->ui_scroll_valid,
        },
    };

    ui->ui_scroll_delta_x = 0;
    ui->ui_scroll_delta_y = 0;
    ui->ui_scroll_valid = false;
    ui->ui_mouse_was_down = is_down;
}

void ui_sdl3_clear(SDL3Ui* ui)
{
    Color color = ui->medit->config.color_theme.editor_bg;
    SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(color));
    SDL_RenderClear(ui->renderer);
}

void ui_sdl3_draw_panel(SDL3Ui* ui, Panel panel, Color bg)
{
    Color border = ui->medit->config.color_theme.panel_border;
    ui_panel(&ui->ui_ctx_bg, panel.area, bg);
    ui_panel(&ui->ui_ctx_bg, panel.separator, border);
}

void ui_sdl3_draw_text(
    SDL3Ui* ui,
    const char* text,
    size_t len,
    Font* font,
    PixelPos pos,
    Color color)
{
    if (text == NULL || len == 0) {
        return;
    }

    TTF_Text* text_obj = ui->text_cache;

    TTF_SetTextFont(text_obj, font->main);
    TTF_SetTextString(text_obj, text, len);
    TTF_SetTextColor(text_obj, color_to_RGBA_args(color));

    TTF_DrawRendererText(text_obj, (float)pos.x, (float)pos.y + (float)font->line_centering_offset);
}

void ui_sdl3_flush_draw_list(SDL3Ui* ui, const UiDrawCmdList* list)
{
    for (size_t i = 0; i < list->count; i++) {
        const UiDrawCmd* cmd = &list->items[i];
        switch (cmd->kind) {
            case UI_CMD_RECT_FILLED: {
                SDL_FRect r = rect_to_sdl_frect(cmd->rect);
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->color));
                SDL_RenderFillRect(ui->renderer, &r);
            } break;
            case UI_CMD_RECT_OUTLINED: {
                SDL_FRect r = rect_to_sdl_frect(cmd->rect);
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->outline_color));
                SDL_RenderRect(ui->renderer, &r);
            } break;
            case UI_CMD_TEXT: {
                ui_sdl3_draw_text(
                    ui,
                    cmd->text,
                    strlen(cmd->text),
                    &ui->font_editor,
                    (PixelPos) { .x = (int)cmd->rect.x, .y = (int)cmd->rect.y },
                    cmd->color);
            } break;
            case UI_CMD_CLIP_PUSH: {
                SDL_Rect r = rect_to_sdl_rect(cmd->rect);
                SDL_SetRenderClipRect(ui->renderer, &r);
            } break;
            case UI_CMD_CLIP_POP: {
                SDL_SetRenderClipRect(ui->renderer, NULL);
            } break;
            case UI_CMD_SCROLLBAR: {
                SDL_SetRenderDrawBlendMode(ui->renderer, SDL_BLENDMODE_BLEND);
                SDL_FRect track = rect_to_sdl_frect(cmd->rect);
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->color));
                SDL_RenderFillRect(ui->renderer, &track);
                SDL_FRect thumb;
                if (cmd->is_horizontal) {
                    float track_w = (float)cmd->rect.w;
                    float thumb_w = cmd->thumb_ratio * track_w;
                    thumb = (SDL_FRect) {
                        .x = track.x + (cmd->scroll_pos * (track_w - thumb_w)),
                        .y = track.y,
                        .w = thumb_w,
                        .h = track.h,
                    };
                } else {
                    float track_h = (float)cmd->rect.h;
                    float thumb_h = cmd->thumb_ratio * track_h;
                    thumb = (SDL_FRect) {
                        .x = track.x,
                        .y = track.y + (cmd->scroll_pos * (track_h - thumb_h)),
                        .w = track.w,
                        .h = thumb_h,
                    };
                }
                SDL_SetRenderDrawColor(ui->renderer, color_to_RGBA_args(cmd->thumb_color));
                SDL_RenderFillRect(ui->renderer, &thumb);
                SDL_SetRenderDrawBlendMode(ui->renderer, SDL_BLENDMODE_NONE);
            } break;
        }
    }
}

void ui_sdl3_render_frame(SDL3Ui* ui)
{
    SDL_RenderPresent(ui->renderer);
}
