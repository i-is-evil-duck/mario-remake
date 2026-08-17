#include "defs/backend/sprite/render.h"

#include "defs/backend/state.h"

auto backend::sprite::cartesian_to_sdl3_coordinates(const glm::vec2& coordinates, const glm::vec2& renderer_size) noexcept -> glm::vec2 {
    return glm::vec2{ coordinates.x + renderer_size.x * 0.5f, -coordinates.y + renderer_size.y * 0.5f };
}

auto backend::sprite::sdl3_to_cartesian_coordinates(const glm::vec2& coordinates, const glm::vec2& renderer_size) noexcept -> glm::vec2 {
    return glm::vec2{ coordinates.x - renderer_size.x * 0.5f, renderer_size.y * 0.5f - coordinates.y };
}

auto backend::sprite::get_image_draw_command(
    const glm::vec2& coordinates, const glm::mat2x2& transform, const glm::vec2& image_size, const std::array<::SDL_FPoint, 4>& uv_coordinates
) noexcept -> std::array<::SDL_Vertex, 4> {
    const glm::vec2 half_size{ image_size / 2.f };
    const glm::vec2 renderer_size{ backend::State::instance().renderer_size };

    // corners of the image, transformed
    const std::array corners{
        // top left
        backend::sprite::cartesian_to_sdl3_coordinates(coordinates + glm::vec2{ -half_size.x, half_size.y } * transform, renderer_size),

        // top right
        backend::sprite::cartesian_to_sdl3_coordinates(coordinates + glm::vec2{ half_size.x, half_size.y } * transform, renderer_size),

        // bottom left
        backend::sprite::cartesian_to_sdl3_coordinates(coordinates + glm::vec2{ -half_size.x, -half_size.y } * transform, renderer_size),

        // bottom right
        backend::sprite::cartesian_to_sdl3_coordinates(coordinates + glm::vec2{ half_size.x, -half_size.y } * transform, renderer_size)
    };

    return {
        ::SDL_Vertex{
            .position  = ::SDL_FPoint{ corners[0].x, corners[0].y },
            .color     = ::SDL_FColor{ .r = 1, .g = 1, .b = 1, .a = 1 },
            .tex_coord = uv_coordinates[0],
        },
        ::SDL_Vertex{
            .position  = ::SDL_FPoint{ corners[1].x, corners[1].y },
            .color     = ::SDL_FColor{ .r = 1, .g = 1, .b = 1, .a = 1 },
            .tex_coord = uv_coordinates[1],
        },
        ::SDL_Vertex{
            .position  = ::SDL_FPoint{ corners[2].x, corners[2].y },
            .color     = ::SDL_FColor{ .r = 1, .g = 1, .b = 1, .a = 1 },
            .tex_coord = uv_coordinates[2],
        },
        ::SDL_Vertex{
            .position  = ::SDL_FPoint{ corners[3].x, corners[3].y },
            .color     = ::SDL_FColor{ .r = 1, .g = 1, .b = 1, .a = 1 },
            .tex_coord = uv_coordinates[3],
        },
    };
}
