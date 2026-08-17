#pragma once

#include "defs/backend/entity.h"
#include "defs/backend/sprite/tileatlas.h"
#include "defs/types.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>

namespace sprite {
    class Tiles final : public backend::Entity {
    public:

        auto start(::SDL_Renderer* const renderer) noexcept -> void;
        auto update(void) noexcept -> void;
        auto render(void) noexcept -> void;
        auto quit(void) noexcept -> void;

    private:

        struct Tile final {
            backend::sprite::TileAtlas::Tile tile{};

            glm::vec2   position{};
            glm::mat2x2 transform{};

            usize tile_index{};

            Tile(backend::sprite::TileAtlas::Tile tile, glm::vec2 position, glm::mat2x2 transform, usize tile_index) :
                tile{ std::move(tile) }, position{ position }, transform{ transform }, tile_index{ tile_index } { }
        };

        struct M {
            backend::sprite::TileAtlas atlas{};

            std::vector<Tile> tiles{};
        } m;

        static constexpr u64 SIZE{ 16 };
    };
}
