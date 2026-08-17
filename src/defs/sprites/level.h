#pragma once

#include "defs/types.h"

#include <glm/glm.hpp>

namespace sprite {
    /**
     * @brief Generate an empty level
     */
    auto generate_level() -> void;

    /**
    * @brief Get the tile index at a world coordinate.
    *
    * @param position World position.
    *
    * @returns Index of the tile in the `level` vector.
    * @see src/impl/sprites/state.h
    */
    auto word_coordinate_to_tile_index(const glm::vec2 position) noexcept -> usize;
}
