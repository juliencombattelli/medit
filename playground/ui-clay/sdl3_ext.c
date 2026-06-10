#include "sdl3_ext.h"

#define CIRCLE_SEGMENTS_COUNT 16
#define CORNER_COUNT 4

void SDL_Ext_RenderFillRoundedRect(SDL_Renderer* renderer, SDL_FRect rect, float corner_radius, SDL_FColor color)
{
    const float min_radius = SDL_min(rect.w, rect.h) / 2.0f;
    const float clamped_radius = SDL_min(corner_radius, min_radius);

    const int circle_segments_count = SDL_max(CIRCLE_SEGMENTS_COUNT, (int)(clamped_radius * 0.5f));

    const int total_vertices = 4 + (4 * circle_segments_count * 2) + (2 * 4);
    const int total_indices = 6 + (4 * circle_segments_count * 3) + (6 * 4);

    SDL_Vertex vertices[total_vertices];
    int indices[total_indices];

    int index_count = 0;
    int vertex_count = 0;

    // define center rectangle
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + clamped_radius, rect.y + clamped_radius },
        color,
        { 0, 0 },
    }; // 0 center TL
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + rect.w - clamped_radius, rect.y + clamped_radius },
        color,
        { 1, 0 },
    }; // 1 center TR
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + rect.w - clamped_radius, rect.y + rect.h - clamped_radius },
        color,
        { 1, 1 },
    }; // 2 center BR
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + clamped_radius, rect.y + rect.h - clamped_radius },
        color,
        { 0, 1 },
    }; // 3 center BL

    indices[index_count++] = 0;
    indices[index_count++] = 1;
    indices[index_count++] = 3;
    indices[index_count++] = 1;
    indices[index_count++] = 2;
    indices[index_count++] = 3;

    // define rounded corners as triangle fans
    const float step = (SDL_PI_F / 2) / (float)circle_segments_count;
    for (int i = 0; i < circle_segments_count; i++) {
        const float angle1 = (float)i * step;
        const float angle2 = ((float)i + 1.0f) * step;

        for (int j = 0; j < CORNER_COUNT; j++) {
            float cx = 0;
            float cy = 0;
            float sign_x = 0;
            float sign_y = 0;

            switch (j) {
                case 0:
                    cx = rect.x + clamped_radius;
                    cy = rect.y + clamped_radius;
                    sign_x = -1;
                    sign_y = -1;
                    break; // top-left
                case 1:
                    cx = rect.x + rect.w - clamped_radius;
                    cy = rect.y + clamped_radius;
                    sign_x = 1;
                    sign_y = -1;
                    break; // top-right
                case 2:
                    cx = rect.x + rect.w - clamped_radius;
                    cy = rect.y + rect.h - clamped_radius;
                    sign_x = 1;
                    sign_y = 1;
                    break; // bottom-right
                case 3:
                    cx = rect.x + clamped_radius;
                    cy = rect.y + rect.h - clamped_radius;
                    sign_x = -1;
                    sign_y = 1;
                    break; // bottom-left
                default: return;
            }

            vertices[vertex_count++] = (SDL_Vertex) {
                {
                    cx + (SDL_cosf(angle1) * clamped_radius * sign_x),
                    cy + (SDL_sinf(angle1) * clamped_radius * sign_y),
                },
                color,
                { 0, 0 },
            };
            vertices[vertex_count++] = (SDL_Vertex) {
                {
                    cx + (SDL_cosf(angle2) * clamped_radius * sign_x),
                    cy + (SDL_sinf(angle2) * clamped_radius * sign_y),
                },
                color,
                { 0, 0 },
            };

            indices[index_count++] = j; // connect to corresponding central rectangle vertex
            indices[index_count++] = vertex_count - 2;
            indices[index_count++] = vertex_count - 1;
        }
    }

    // define edge rectangles

    //  top edge
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + clamped_radius, rect.y },
        color,
        { 0, 0 },
    }; // TL
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + rect.w - clamped_radius, rect.y },
        color,
        { 1, 0 },
    }; // TR
    indices[index_count++] = 0;
    indices[index_count++] = vertex_count - 2; // TL
    indices[index_count++] = vertex_count - 1; // TR
    indices[index_count++] = 1;
    indices[index_count++] = 0;
    indices[index_count++] = vertex_count - 1; // TR

    // right edge
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + rect.w, rect.y + clamped_radius },
        color,
        { 1, 0 },
    }; // RT
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + rect.w, rect.y + rect.h - clamped_radius },
        color,
        { 1, 1 },
    }; // RB
    indices[index_count++] = 1;
    indices[index_count++] = vertex_count - 2; // RT
    indices[index_count++] = vertex_count - 1; // RB
    indices[index_count++] = 2;
    indices[index_count++] = 1;
    indices[index_count++] = vertex_count - 1; // RB

    // bottom edge
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + rect.w - clamped_radius, rect.y + rect.h },
        color,
        { 1, 1 },
    }; // BR
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x + clamped_radius, rect.y + rect.h },
        color,
        { 0, 1 },
    }; // BL
    indices[index_count++] = 2;
    indices[index_count++] = vertex_count - 2; // BR
    indices[index_count++] = vertex_count - 1; // BL
    indices[index_count++] = 3;
    indices[index_count++] = 2;
    indices[index_count++] = vertex_count - 1; // BL

    // left edge
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x, rect.y + rect.h - clamped_radius },
        color,
        { 0, 1 },
    }; // LB
    vertices[vertex_count++] = (SDL_Vertex) {
        { rect.x, rect.y + clamped_radius },
        color,
        { 0, 0 },
    }; // LT
    indices[index_count++] = 3;
    indices[index_count++] = vertex_count - 2; // LB
    indices[index_count++] = vertex_count - 1; // LT
    indices[index_count++] = 0;
    indices[index_count++] = 3;
    indices[index_count++] = vertex_count - 1; // LT

    // render everything
    SDL_RenderGeometry(renderer, NULL, vertices, vertex_count, indices, index_count);
}
