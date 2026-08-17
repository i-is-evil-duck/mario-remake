#pragma once

#include "defs/backend/entity.h"
#include "defs/backend/sprite/tileatlas.h"

#include <SDL3/SDL.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <vector>

namespace sprite {
    class Backgrounds final : public backend::Entity {
    public:

        auto start(::SDL_Renderer* const renderer) noexcept -> void;
        auto update(void) noexcept -> void;
        auto render(void) noexcept -> void;
        auto quit(void) noexcept -> void;

    private:

        struct Background final {
            backend::sprite::TileAtlas::Tile tile{};

            glm::vec2   position{};
            glm::mat2x2 transform{};

            Background(backend::sprite::TileAtlas::Tile tile, glm::vec2 position, glm::mat2x2 transform) :
                tile{ std::move(tile) }, position{ position }, transform{ transform } { }
        };

        struct M {
            backend::sprite::TileAtlas atlas{};

            glm::vec2 grid_size{};
            f64       y_start{};

            std::vector<Background> backgrounds{};
        } m;

        static constexpr u64 SIZE{ 512 };

        const std::filesystem::path BACKGROUNDS_PATH{ "res/images/background-tileset.png" };
        const std::filesystem::path BACKGROUNDS_MAP{ "res/tile-map/background-tileset.toml" };
    };
}
