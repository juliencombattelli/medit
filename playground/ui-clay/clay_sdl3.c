#define CLAY_IMPLEMENTATION
#include "clay.h"

#include "clay_sdl3.h"
#include "sdl3_ext.h"

void SDL_Clay_RenderClayCommands(SDL_Renderer* renderer, Clay_RenderCommandArray draw_commands)
{
    for (int32_t i = 0; i < draw_commands.length; i++) {
        Clay_RenderCommand* rcmd = Clay_RenderCommandArray_Get(&draw_commands, i);
        const Clay_BoundingBox bounding_box = rcmd->boundingBox;
        const SDL_FRect rect = {
            .x = (float)bounding_box.x,
            .y = (float)bounding_box.y,
            .w = (float)bounding_box.width,
            .h = (float)bounding_box.height,
        };

        switch (rcmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* config = &rcmd->renderData.rectangle;
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                Clay_Color bg = rcmd->renderData.rectangle.backgroundColor;
                SDL_SetRenderDrawColor(
                    renderer,
                    (Uint8)bg.r,
                    (Uint8)bg.g,
                    (Uint8)bg.b,
                    (Uint8)bg.a);
                if (config->cornerRadius.topLeft > 0) {
                    SDL_Ext_RenderFillRoundedRect(
                        renderer,
                        rect,
                        config->cornerRadius.topLeft,
                        clay_to_sdl_color(config->backgroundColor));
                } else {
                    SDL_RenderFillRect(renderer, &rect);
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                Clay_BoundingBox boundingBox = rcmd->boundingBox;
                SDL_Rect currentClippingRectangle = {
                    .x = (int)boundingBox.x,
                    .y = (int)boundingBox.y,
                    .w = (int)boundingBox.width,
                    .h = (int)boundingBox.height,
                };
                SDL_SetRenderClipRect(renderer, &currentClippingRectangle);
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                SDL_SetRenderClipRect(renderer, NULL);
            } break;
        }
    }
}
