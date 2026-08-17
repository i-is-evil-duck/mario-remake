#pragma once

#include "defs/types.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace backend {
    class Entity {
    public:

        Entity(void) noexcept;
        virtual ~Entity() noexcept;

        /**
         * @brief Setup entity
         *
         * @param renderer SDL3 renderer
         */
        virtual auto start(::SDL_Renderer* const renderer) noexcept -> void = 0;

        /**
         * @brief Update, must return once per "frame" and will be restored
         */
        virtual auto update(void) noexcept -> void = 0;

        /**
         * @brief Render image
         */
        virtual auto render(void) noexcept -> void = 0;

        /**
         * @brief Destroy entity
         */
        virtual auto quit(void) noexcept -> void = 0;

        b8    hidden{};
        usize object_layer{};

    protected:

        glm::vec2   position{};
        glm::mat2x2 transform{ 1 };
    };
}
