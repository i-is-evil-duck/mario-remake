#pragma once

#include "defs/autorelease.h"
#include "defs/types.h"

#include <SDL3/SDL.h>
#include <filesystem>
#include <glm/glm.hpp>

namespace backend::sprite {
    class Image final {
    public:

        Image(void) noexcept;
        ~Image() noexcept;

        Image(const sprite::Image&) noexcept;
        auto operator=(const sprite::Image&) noexcept -> sprite::Image&;

        Image(sprite::Image&&) noexcept;
        auto operator=(sprite::Image&&) noexcept -> sprite::Image&;

        /**
         * @brief Create image
         *
         * @param renderer SDL window renderer
         * @param path Path to image
         */
        auto create(::SDL_Renderer* const renderer, const std::filesystem::path& path) noexcept -> void;

        /**
         * @brief Destroy image
         */
        auto destroy(void) noexcept -> void;

        /**
         * @brief Get size of texture
         *
         * @return Size of texture
         */
        auto size(void) const noexcept -> glm::vec2;

        /**
         * @brief Get texture
         */
        auto get(void) const noexcept -> ::SDL_Texture*;

        /**
         * @brief Render image to renderer given in `create`
         *
         * @param position Vector of position
         * @param transform Optional transform to give image
         */
        auto render(const glm::vec2& position, const glm::mat2x2& transform = 1) const noexcept -> void;

    private:

        struct M {
            b8 initialized{};

            std::filesystem::path      path{};
            ::SDL_Renderer*            renderer{};
            glm::vec2                  size{};
            AutoRelease<::SDL_Texture> texture{};
        } m;
    };
}
