#pragma once

#include "defs/sprites/settings.h"
#include "defs/types.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace sprite {
    class State final {
    public:

        ~State() noexcept;

        State(const sprite::State&) noexcept                            = delete;
        auto operator=(const sprite::State&) noexcept -> sprite::State& = delete;

        State(sprite::State&&) noexcept                            = delete;
        auto operator=(sprite::State&&) noexcept -> sprite::State& = delete;

        static auto instance(void) noexcept -> sprite::State&;

        f64 scaled_tile{};

        const std::filesystem::path TILES_PATH{ "res/images/block-tileset.png" };
        const std::filesystem::path TILES_MAP{ "res/tile-map/block-tileset.toml" };

        b8                       is_editing{};
        b8                       tiles_need_to_reset{ true };
        glm::vec2                camera_position{};
        glm::vec2                grid_size{};
        std::vector<std::string> level{};
        glm::vec2                level_size{ 100, 40 };
        sprite::Settings         settings{};

    private:

        State(void) noexcept;
    };
}
