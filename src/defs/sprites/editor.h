#pragma once

#include "defs/backend/entity.h"
#include "defs/backend/sprite/tileatlas.h"
#include "defs/types.h"

#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace sprite {
    class Editor final : public backend::Entity {
    public:

        auto start(::SDL_Renderer* const renderer) noexcept -> void;
        auto update(void) noexcept -> void;
        auto render(void) noexcept -> void;
        auto quit(void) noexcept -> void;

    private:

        /**
         * @brief Load the TOML block groups and variants
         */
        auto load_block_data(void) noexcept -> void;

        /**
         * @brief Load/save a level
         */
        auto sync_level(void) noexcept -> void;

        /**
         * @brief Show a brush
         */
        auto show_brush(void) noexcept -> void;

        /**
         * @brief Swap brushes
         */
        auto swap_brushes(void) noexcept -> void;

        /**
         * Update brush to the next item
         *
         * @param group_index Group to update brush in
         */
        auto update_brush(const usize group_index) noexcept -> void;

        /**
         * @brief Draw tiles
         */
        auto draw_tiles(void) noexcept -> void;

    private:

        struct M {
            backend::sprite::TileAtlas       tiles{};
            backend::sprite::TileAtlas::Tile brush_tile{};

            std::vector<std::string> tile_groups{};
            std::vector<usize>       tile_group_boundaries{};
            usize                    tile_group_brush{};

            std::string selected_tile{};

            std::unordered_map<std::string, usize> variants{};
        } m;

        const std::filesystem::path      BLOCK_GROUPS_PATH{ "res/block-data/groups.toml" };
        const std::array<std::string, 6> BLOCK_GROUPS_ORDER = { "unordered", "pipes", "semisolid", "mushrooms", "decorations", "ground" };
        const std::filesystem::path      GROUND_VARIANTS_PATH{ "res/block-data/variants.toml" };
    };
}
