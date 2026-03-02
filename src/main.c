#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define FONT_SIZE 18

typedef struct {
    SDL_Renderer* renderer;
    TTF_Font* font;
    int char_width;
    int char_height;
    int cursor_x;
    int cursor_y;
} Meditor;

void sdl_assert(bool condition, const char* sdl_call)
{
    if (!condition) {
        fprintf(stderr, "Error: %s: %s\n", sdl_call, SDL_GetError());
        exit(1);
    }
}

void render_text0(
    Meditor* meditor,
    const char* text,
    int cell_x,
    int cell_y,
    SDL_Color color)
{
    SDL_Surface* surface = TTF_RenderText_Blended(
        meditor->font,
        text,
        0,
        text ? color : (SDL_Color) { .r = 255, .g = 255, .b = 255, .a = 255 });
    sdl_assert(surface, "TTF_RenderText_Blended");

    SDL_Texture* texture = SDL_CreateTextureFromSurface(
        meditor->renderer,
        surface);
    sdl_assert(texture, "SDL_CreateTextureFromSurface");

    SDL_FRect dest = {
        .x = (float)(cell_x * meditor->char_width),
        .y = (float)(cell_y * meditor->char_height),
        .w = (float)surface->w,
        .h = (float)surface->h,
    };
    SDL_RenderTexture(meditor->renderer, texture, NULL, &dest);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

void render_cursor(Meditor* meditor, SDL_Color color)
{
    const SDL_FRect rect = {
        .x = (float)(meditor->cursor_x * meditor->char_width),
        .y = (float)(meditor->cursor_y * meditor->char_height),
        .w = (float)meditor->char_width,
        .h = (float)meditor->char_height,
    };

    SDL_SetRenderDrawColor(
        meditor->renderer,
        color.r,
        color.g,
        color.b,
        color.a);
    SDL_RenderFillRect(meditor->renderer, &rect);
}

int main(int argc, char** argv)
{
    sdl_assert(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
    sdl_assert(TTF_Init(), "TTF_Init");

    SDL_Window* window = SDL_CreateWindow("Med", 1280, 720, SDL_WINDOW_HIDDEN);
    sdl_assert(window, "SDL_CreateWindow");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    sdl_assert(renderer, "SDL_CreateRenderer");
    SDL_SetRenderVSync(renderer, 1);

    // Load font
    TTF_Font* font = TTF_OpenFont("asset/font/Consola.ttf", FONT_SIZE);
    sdl_assert(font, "TTF_OpenFont");

    Meditor meditor = {
        .renderer = renderer,
        .font = font,
        .char_width = 0,
        .char_height = 0,
        .cursor_x = 0,
        .cursor_y = 0,
    };

    // Measure character dimensions once
    TTF_GetStringSize(font, "M", 0, &meditor.char_width, &meditor.char_height);

    bool running = true;
    SDL_ShowWindow(window);

    SDL_StartTextInput(window);

#define TEXT_CAPACITY (size_t)(1024 * 1024)
    char text[TEXT_CAPACITY] = { 0 };
    size_t text_size = 0;

    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event) != 0) {
            int win_width = 0;
            int win_height = 0;
            SDL_GetWindowSize(window, &win_width, &win_height);
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                    if (event.key.scancode == SDL_SCANCODE_UP) {
                        if (meditor.cursor_y != 0) {
                            meditor.cursor_y -= 1;
                        }
                    }
                    if (event.key.scancode == SDL_SCANCODE_DOWN) {
                        if ((meditor.cursor_y + 1) * meditor.char_height
                            < win_height) {
                            meditor.cursor_y += 1;
                        }
                    }
                    if (event.key.scancode == SDL_SCANCODE_LEFT) {
                        if (meditor.cursor_x != 0) {
                            meditor.cursor_x -= 1;
                        }
                    }
                    if (event.key.scancode == SDL_SCANCODE_RIGHT) {
                        if ((meditor.cursor_x + 1) * meditor.char_width
                            < win_width) {
                            meditor.cursor_x += 1;
                        }
                    }
                } break;
                case SDL_EVENT_TEXT_INPUT: {
                    size_t input_size = strlen(event.text.text);
                    const size_t free_space = TEXT_CAPACITY - text_size;
                    if (text_size > free_space) {
                        text_size = free_space;
                    }
                    memcpy(text + text_size, event.text.text, input_size);
                    text_size += input_size;
                    int text_width = 0;
                    // TODO TTF_GetStringSize only on the input string and
                    // increment the cursor
                    TTF_GetStringSize(font, text, text_size, &text_width, NULL);
                    meditor.cursor_x = text_width / meditor.char_width;
                } break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Render text examples
        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Color cyan = { 0, 255, 255, 255 };
        SDL_Color lime = { 0, 255, 0, 255 };

        render_text0(&meditor, "TrueType Font Rendering", 10, 4, white);
        render_text0(&meditor, "With SDL_ttf", 10, 6, cyan);
        render_text0(&meditor, "Press ESC to exit", 10, 10, lime);

        if (text_size != 0) {
            render_text0(&meditor, text, 0, 0, white);
        }
        render_cursor(&meditor, white);

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window);

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}
