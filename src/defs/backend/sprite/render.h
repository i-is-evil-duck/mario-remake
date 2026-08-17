#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <glm/glm.hpp>

namespace backend::sprite {
    /**
     * @brief Transform cartesian coordinates to SDL3 coordinates
     *
     * @param coordinates Cartesian coordinates
     * @param renderer_size Size of the renderer
     *
     * @return SDL3 coordinates
     */
    auto cartesian_to_sdl3_coordinates(const glm::vec2& coordinates, const glm::vec2& renderer_size) noexcept -> glm::vec2;

    /**
     * @brief Transform SDL3 coordinates to cartesian coordinates
     *
     * @param coordinates SDL3 coordinates
     * @param renderer_size Size of the renderer
     *
     * @return Cartesian coordinates
     */
    auto sdl3_to_cartesian_coordinates(const glm::vec2& coordinates, const glm::vec2& renderer_size) noexcept -> glm::vec2;

    /**
     * @brief Get the vertex information for an image
     *
     * @param coordinates Coordinates to render image (in SDL space)
     * @param transform Matrix to multiply image by
     * @param image_size Size of texture
     * @param uv_coordinates Coordinates of texture to sample from, in order `{ TL, TR, BL, BR }`
     */
    auto get_image_draw_command(
        const glm::vec2&                   coordinates,
        const glm::mat2x2&                 transform,
        const glm::vec2&                   image_size,
        const std::array<::SDL_FPoint, 4>& uv_coordinates
    ) noexcept -> std::array<::SDL_Vertex, 4>;
}
