#include "sdl3_internal.h"

#include "utils/font.h"

#include <core/assert.h>
#include <core/safeint.h>
#include <core/utils.h>

void ui_sdl3_ttf_setup(SDL3Ui* ui)
{
    // The entire SDL_ttf subsystem is (re)created on every entry into the app
    // library, including after a hot reload. libSDL3_ttf.so is referenced only
    // by this hot-reloadable library, so the loader's dlclose drops its
    // refcount to zero and the dynamic linker reloads it at a NEW address on the
    // next reload. Any SDL_ttf object (font, text, text engine and their
    // internal hashtables) that survived the reload would still hold function
    // pointers into the old, now-unmapped libSDL3_ttf.so and crash when used
    // (e.g. a wild jump through a stale hashtable hash callback or engine
    // vtable). Recreating everything here rebinds every internal pointer to the
    // currently mapped library.
    if (!TTF_Init()) {
        printf("Failed to initialize SDL_ttf: %s\n", SDL_GetError());
        abort();
    }

    ui->text_engine = TTF_CreateRendererTextEngine(ui->renderer);
    assert(ui->text_engine != NULL);

    for (FontId id = 0; id < FONT_ID_COUNT; id++) {
        ui_sdl3_load_font(ui, id);
    }
}

void ui_sdl3_ttf_teardown(SDL3Ui* ui)
{
    for (FontId id = 0; id < FONT_ID_COUNT; id++) {
        ui_sdl3_unload_font(ui, id);
    }

    TTF_DestroyRendererTextEngine(ui->text_engine);
    ui->text_engine = NULL;

    TTF_Quit();
}

void ui_sdl3_load_font(SDL3Ui* ui, FontId font_id)
{
    assert(font_id >= 0 && font_id < FONT_ID_COUNT);

    Meditor* medit = ui->medit;
    Font* font = &ui->fonts[font_id];

    if (font->props == 0) {
        font->props = SDL_CreateProperties();
        assert(font->props != 0);
    }

    SDL_SetStringProperty(
        font->props,
        TTF_PROP_FONT_CREATE_FILENAME_STRING,
        medit->config.editor_font_path);
    SDL_SetFloatProperty(
        font->props,
        TTF_PROP_FONT_CREATE_SIZE_FLOAT,
        (float)medit->config.editor_font_size);
    SDL_SetNumberProperty(
        font->props,
        TTF_PROP_FONT_CREATE_HORIZONTAL_DPI_NUMBER,
        FONT_DPI_DEFAULT);
    SDL_SetNumberProperty(
        font->props,
        TTF_PROP_FONT_CREATE_VERTICAL_DPI_NUMBER,
        FONT_DPI_DEFAULT);

    font->main = TTF_OpenFontWithProperties(font->props);
    if (!font->main) {
        printf(
            "Failed to load editor font (size %d): %s\n",
            medit->config.editor_font_size,
            SDL_GetError());
        abort();
    }

    if (!TTF_FontIsFixedWidth(font->main)) {
        printf("Warning: editor font '%s' is not fixed-width\n", medit->config.editor_font_path);
    }

    int line_spacing = TTF_GetFontLineSkip(font->main);
    font->line_spacing = int_to_size(line_spacing);
    int font_h = TTF_GetFontHeight(font->main);
    font->line_centering_offset = (line_spacing - font_h + 1) / 2;

    int w = 0;
    assert(TTF_GetStringSize(font->main, "M", 0, &w, NULL));
    font->default_cursor_width = int_to_size(w);

    ui_sdl3_resize_window(ui);

    ui->text_cache = TTF_CreateText(ui->text_engine, font->main, "", 0);
    assert(ui->text_cache != NULL);

    const int width_factor = 0; // Do not align the emoji font width to the main font width
    font->emoji = load_emoji_font_aligned_to(
        font->main,
        // "asset/font/NotoColorEmoji-Regular.ttf",
        "asset/font/OpenMoji-color-colr0_svg.ttf",
        // "asset/font/seguiemj.ttf",
        medit->config.editor_font_size,
        width_factor);
    if (!font->emoji) {
        printf("Warning: failed to load fallback emoji font: %s\n", SDL_GetError());
    } else {
        if (!TTF_AddFallbackFont(font->main, font->emoji)) {
            printf("Warning: failed to add fallback emoji font: %s\n", SDL_GetError());
        }
    }
}

void ui_sdl3_unload_font(SDL3Ui* ui, FontId font_id)
{
    assert(font_id >= 0 && font_id < FONT_ID_COUNT);

    Font* font = &ui->fonts[font_id];

    TTF_DestroyText(ui->text_cache);
    ui->text_cache = NULL;

    TTF_ClearFallbackFonts(font->main);
    TTF_CloseFont(font->main);
    TTF_CloseFont(font->emoji);
    *font = (Font) { 0 };
}
