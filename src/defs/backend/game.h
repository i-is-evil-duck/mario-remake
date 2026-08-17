#pragma once

#include "defs/backend/entity.h"
#include "defs/types.h"

#include <SDL3/SDL.h>
#include <memory>
#include <utility>
#include <vector>

namespace backend {
    class Game final {
    public:

        Game(void) noexcept;
        ~Game(void) noexcept;

        Game(const backend::Game&) noexcept                             = delete;
        auto operator=(const backend::Game&) noexcept -> backend::Game& = delete;

        Game(backend::Game&&) noexcept                             = delete;
        auto operator=(backend::Game&&) noexcept -> backend::Game& = delete;

        /**
         * @brief Create game manager
         */
        auto create(::SDL_Renderer* const renderer) noexcept -> void;

        /**
         * @brief Destroy game manager
         */
        auto destroy(void) noexcept -> void;

        /**
         * @brief Run game for one tick
         */
        auto tick(void) noexcept -> void;

    private:

        struct M {
            b8 initialized{};

            std::vector<std::unique_ptr<backend::Entity>> entities{};
            std::vector<std::pair<usize, usize>>          layer_sorted_entities{};
        } m;
    };
}
