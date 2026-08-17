#pragma once

#include "defs/backend/sprite/image.h"
#include "defs/types.h"

#include <SDL3/SDL.h>
#include <array>
#include <filesystem>
#include <vector>

namespace backend::sprite {
    class TileAtlas final {
    public:

        class Tile;

        TileAtlas(void) noexcept;
        ~TileAtlas() noexcept;

        TileAtlas(const backend::sprite::TileAtlas&) noexcept                                     = delete;
        auto operator=(const backend::sprite::TileAtlas&) noexcept -> backend::sprite::TileAtlas& = delete;

        TileAtlas(backend::sprite::TileAtlas&&) noexcept                                     = delete;
        auto operator=(backend::sprite::TileAtlas&&) noexcept -> backend::sprite::TileAtlas& = delete;

        /**
         * @brief Create a tile atlas, should only be called once
         *
         * @param renderer SDL3 renderer
         * @param atlas_path Path to tile atlas
         * @param configuration_path Path to configuration of tile atlas
         */
        auto
        create(::SDL_Renderer* const renderer, const std::filesystem::path& map_path, const std::filesystem::path& configuration_path) noexcept
            -> void;

        /**
         * @brief Destroy tile atlas
         */
        auto destroy(void) noexcept -> void;

        /**
         * @brief Get a tile from a tile atlas
         *
         * @param name Name of tile in tile atlas, set in TOML configuration
         *
         * @return Tile
         */
        auto get(const std::string_view& name) noexcept -> Tile;

    public:

        class Tile final {
        public:

            Tile(void) noexcept;
            ~Tile() noexcept;

            Tile(const backend::sprite::TileAtlas::Tile&) noexcept;
            auto operator=(const sprite::TileAtlas::Tile&) noexcept -> sprite::TileAtlas::Tile&;

            Tile(sprite::TileAtlas::Tile&&) noexcept;
            auto operator=(sprite::TileAtlas::Tile&&) noexcept -> sprite::TileAtlas::Tile&;

            /**
             * @brief Destroy tile
             */
            auto destroy(void) noexcept -> void;

            /**
             * @brief Render a tile
             *
             * @param position Vector of position
             * @param transform Optional transform to give image
             * @param transparency Transparency of the tile, must be in [0, 1]
             */
            auto render(const glm::vec2& position, const glm::mat2x2& transform = 1, f32 transparency = 1) noexcept -> void;

            /**
            * @brief Get size of texture
            *
            * @return Size of texture
            */
            auto size(void) const noexcept -> glm::vec2;

        private:

            friend backend::sprite::TileAtlas;

            /**
             * @brief Create a sprite
             *
             * @param renderer Renderer
             * @param texture tile map
             * @param size Size of texture
             * @param uv_coordinates Coordinates of tile in order `{ TL, TR, BL, BR }`
             * @param vertices_vector Vector containing all vertices that will be rendered
             */
            auto create(
                ::SDL_Renderer* const               renderer,
                ::SDL_Texture* const                texture,
                const glm::vec2&                    size,
                const std::array<::SDL_FPoint, 4>&& uv_coordinates
            ) noexcept -> void;

        private:

            struct M {
                b8 initialized{};

                ::SDL_Renderer*             renderer{};
                ::SDL_Texture*              texture{};
                glm::vec2                   size{};
                std::array<::SDL_FPoint, 4> uv_coordinates{};
            } m;
        };

    private:

        struct TileRect {
            glm::vec2 position{};
            glm::vec2 size{};
        };

        struct M {
            b8 initialized{};

            std::unordered_map<std::string, sprite::TileAtlas::TileRect> configurations{};

            ::SDL_Renderer*           renderer{};
            sprite::Image             texture{};
            std::vector<::SDL_Vertex> vertices{};
        } m;
    };
}
