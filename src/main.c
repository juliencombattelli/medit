#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdio.h>
#include <stdlib.h>

void sdl_assert(bool condition, const char* sdl_call)
{
    if (!condition) {
        fprintf(stderr, "Error: %s: %s\n", sdl_call, SDL_GetError());
        exit(1);
    }
}

void render_text0(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const char* text,
    int x,
    int y,
    SDL_Color color)
{
    SDL_Surface* surface = TTF_RenderText_Blended(
        font,
        text,
        0,
        text ? color : (SDL_Color) { 255, 255, 255, 255 });
    sdl_assert(surface, "TTF_RenderText_Blended");

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    sdl_assert(texture, "SDL_CreateTextureFromSurface");

    SDL_FRect dest
        = { (float)x, (float)y, (float)surface->w, (float)surface->h };
    SDL_RenderTexture(renderer, texture, NULL, &dest);

    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char** argv)
{
    sdl_assert(SDL_Init(SDL_INIT_VIDEO), "SDL_Init");
    sdl_assert(TTF_Init(), "TTF_Init");

    SDL_Window* window = SDL_CreateWindow("Med", 1280, 720, SDL_WINDOW_HIDDEN);
    sdl_assert(window, "SDL_CreateWindow");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    sdl_assert(renderer, "SDL_CreateRenderer");

    // Load font
    TTF_Font* font = TTF_OpenFont("asset/font/Consola.ttf", 16);
    sdl_assert(font, "TTF_OpenFont");

    bool running = true;
    SDL_ShowWindow(window);

    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN: {
                    if (e.key.scancode == SDL_SCANCODE_ESCAPE) {
                        running = false;
                    }
                } break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Render text examples
        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Color cyan = { 0, 255, 255, 255 };
        SDL_Color lime = { 0, 255, 0, 255 };

        render_text0(renderer, font, "TrueType Font Rendering", 50, 50, white);
        render_text0(renderer, font, "With SDL_ttf", 50, 90, cyan);
        render_text0(renderer, font, "Press ESC to exit", 50, 150, lime);

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}
