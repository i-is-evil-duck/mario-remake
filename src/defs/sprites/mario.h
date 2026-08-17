#pragma once

#include "defs/backend/entity.h"
#include "defs/backend/sprite/tileatlas.h"
#include "defs/types.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace sprite {
    class Mario final : public backend::Entity {
    public:

        auto start(::SDL_Renderer* const renderer) noexcept -> void;
        auto update(void) noexcept -> void;
        auto render(void) noexcept -> void;
        auto quit(void) noexcept -> void;

    private:

        /**
         * @brief Set the camera position
         */
        auto set_camera_position(void) noexcept -> void;

        /**
         * @brief Move in the editing mode
         */
        auto move_editing_mode(void) noexcept -> void;

        /**
         * @brief Move in the x direction
         */
        auto move_x_direction(void) noexcept -> void;

        /**
         * @brief Move in the y direction
         */
        auto move_y_direction(void) noexcept -> void;

        /**
         * @brief Resolve collisions in the x axis
         */
        auto resolve_x_collisions(void) noexcept -> void;

        /**
         * @brief Resolve collisions in the y axis
         */
        auto resolve_y_collisions(void) noexcept -> void;

        /**
         * @brief Check if mario is colliding with anything
         *
         * @returns Whether mario is colliding with anything
         */
        auto is_colliding(void) const noexcept -> b8;

        /**
         * @brief Set the sprite of mario
         */
        auto set_mario_skin(void) noexcept -> void;

    private:

        struct M {
            backend::sprite::TileAtlas       atlas{};
            backend::sprite::TileAtlas::Tile sprite{};

            usize frames_falling{};
            usize frames_jumping{};

            f64 sprite_speed{};
            f64 walk_frame{};

            glm::vec2 velocity{};
            glm::vec2 size{ 15, 20 };

            enum class SpecialActions {
                Normal = 0,
                Skidding,
            } special_actions{};
        } m;

        static constexpr f64 TINY{ 0.01 };  // ! do NOT edit. Used for collision checks and FPEs

        const std::filesystem::path MARIO_PATH{ "res/images/character-tileset.png" };
        const std::filesystem::path MARIO_MAP{ "res/tile-map/mario-tileset.toml" };
    };
}
