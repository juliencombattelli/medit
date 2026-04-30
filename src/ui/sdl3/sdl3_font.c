#include "sdl3_internal.h"

#include "utils/font.h"

#include <core/assert.h>
#include <core/safeint.h>
#include <core/utils.h>

void ui_sdl3_load_editor_font(SDL3Ui* ui)
{
    Meditor* medit = ui->medit;

    if (ui->font_editor.props == 0) {
        ui->font_editor.props = SDL_CreateProperties();
        assert(ui->font_editor.props != 0);
    }

    SDL_SetStringProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_FILENAME_STRING,
        medit->config.editor_font_path);
    SDL_SetFloatProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_SIZE_FLOAT,
        (float)medit->config.editor_font_size);
    SDL_SetNumberProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_HORIZONTAL_DPI_NUMBER,
        FONT_DPI_DEFAULT);
    SDL_SetNumberProperty(
        ui->font_editor.props,
        TTF_PROP_FONT_CREATE_VERTICAL_DPI_NUMBER,
        FONT_DPI_DEFAULT);

    ui->font_editor.main = TTF_OpenFontWithProperties(ui->font_editor.props);
    if (!ui->font_editor.main) {
        printf(
            "Failed to load editor font (size %d): %s\n",
            medit->config.editor_font_size,
            SDL_GetError());
        abort();
    }

    if (!TTF_FontIsFixedWidth(ui->font_editor.main)) {
        printf("Warning: editor font '%s' is not fixed-width\n", medit->config.editor_font_path);
    }

    int line_spacing = TTF_GetFontLineSkip(ui->font_editor.main);
    ui->font_editor.line_spacing = int_to_size(line_spacing);
    int font_h = TTF_GetFontHeight(ui->font_editor.main);
    ui->font_editor.line_centering_offset = (line_spacing - font_h + 1) / 2;

    int w = 0;
    assert(TTF_GetStringSize(ui->font_editor.main, "M", 0, &w, NULL));
    ui->font_editor.default_cursor_width = int_to_size(w);

    ui_sdl3_resize_window(ui);

    ui->text_cache = TTF_CreateText(ui->text_engine, ui->font_editor.main, "", 0);
    assert(ui->text_cache != NULL);

    const int width_factor = 0; // Do not align the emoji font width to the main font width
    ui->font_editor.emoji = load_emoji_font_aligned_to(
        ui->font_editor.main,
        // "asset/font/NotoColorEmoji-Regular.ttf",
        "asset/font/OpenMoji-color-colr0_svg.ttf",
        // "asset/font/seguiemj.ttf",
        medit->config.editor_font_size,
        width_factor);
    if (!ui->font_editor.emoji) {
        printf("Warning: failed to load fallback emoji font: %s\n", SDL_GetError());
    } else {
        if (!TTF_AddFallbackFont(ui->font_editor.main, ui->font_editor.emoji)) {
            printf("Warning: failed to add fallback emoji font: %s\n", SDL_GetError());
        }
    }
}

void ui_sdl3_unload_editor_font(SDL3Ui* ui)
{
    TTF_DestroyText(ui->text_cache);
    ui->text_cache = NULL;

    TTF_ClearFallbackFonts(ui->font_editor.main);
    TTF_CloseFont(ui->font_editor.main);
    TTF_CloseFont(ui->font_editor.emoji);
    ui->font_editor = (Font) { 0 };
}
