#pragma once

#include "defs/backend/entity.h"

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

namespace loop {
    /**
     * @brief Run on game start
     *
     * @param renderer SDL3 renderer
     * @param entities Vector of entities to be added to the game, must not call start on any members
     */
    auto start(::SDL_Renderer* const renderer, std::vector<std::unique_ptr<backend::Entity>>& entities) noexcept -> void;

    /**
     * @brief Run on each game frame
     *
     * @param entities Vector of entities to be added to the game, must not call update on any members
     */
    auto update(void) noexcept -> void;

    /**
     * @brief Run on quit
     *
     * @param entities Vector of entities to be added to the game, must not call quit on any members
     */
    auto quit(void) noexcept -> void;
}
