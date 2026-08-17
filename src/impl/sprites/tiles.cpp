#include "defs/sprites/tiles.h"

#include "defs/backend/sprite/tileatlas.h"
#include "defs/backend/state.h"
#include "defs/sprites/settings.h"
#include "defs/sprites/state.h"
#include "defs/types.h"

#include <cstdlib>

auto sprite::Tiles::start(::SDL_Renderer* const renderer) noexcept -> void {
    sprite::State& instance{ sprite::State::instance() };

    object_layer = 10;

    m.atlas.create(renderer, instance.TILES_PATH, instance.TILES_MAP);

    const u64 tile_scale{ static_cast<u64>(instance.settings.get(combine("tile", "scale"))) };
    instance.scaled_tile = SIZE * tile_scale;

    instance.grid_size.x = std::ceil(backend::State::instance().renderer_size.x / instance.scaled_tile) + 1;
    instance.grid_size.y = std::ceil(backend::State::instance().renderer_size.y / instance.scaled_tile) + 1;

    const f64 x_start{ -(static_cast<f64>(instance.grid_size.x - 1) * instance.scaled_tile) / 2. };
    const f64 y_start{ -(static_cast<f64>(instance.grid_size.y - 1) * instance.scaled_tile) / 2. };

    usize tile_index{};

    for (i64 x{}; x < instance.grid_size.x; ++x) {
        for (i64 y{}; y < instance.grid_size.y; ++y) {
            m.tiles.emplace_back(
                m.atlas.get("air"),
                glm::vec2{ x_start + (x * instance.scaled_tile), y_start + (y * instance.scaled_tile) },
                glm::mat2x2{ tile_scale, 0, 0, tile_scale },
                tile_index++
            );
        }

        tile_index += instance.level_size.y - instance.grid_size.y;
    }
}

auto sprite::Tiles::update(void) noexcept -> void {
    sprite::State& instance{ sprite::State::instance() };

    const f64 total_width{ instance.grid_size.x * instance.scaled_tile };
    const f64 total_height{ instance.grid_size.y * instance.scaled_tile };

    for (auto& tile : m.tiles) {
        f64 relative_x{ tile.position.x - instance.camera_position.x };
        f64 relative_y{ tile.position.y - instance.camera_position.y };

        b8 should_redraw{};

        if (relative_x < -total_width / 2.) {                           // if a tile has moved off the edge of the screen
            tile.position.x += total_width;                              // loop it back to the other side
            tile.tile_index += instance.grid_size.x * instance.level_size.y; // move the level index to the right

            should_redraw = true;
        } else if (relative_x > total_width / 2.) {
            tile.position.x -= total_width;
            tile.tile_index -= instance.grid_size.x * instance.level_size.y;

            should_redraw = true;
        }

        if (relative_y < -total_height / 2.) {
            tile.position.y += total_height;
            tile.tile_index += instance.grid_size.y;

            should_redraw = true;
        } else if (relative_y > total_height / 2.) {
            tile.position.y -= total_height;
            tile.tile_index -= instance.grid_size.y;

            should_redraw = true;
        }

        i64 map_size   = static_cast<i64>(instance.level.size());
        i64 safe_index = ((tile.tile_index % map_size) + map_size) % map_size;

        if (instance.tiles_need_to_reset || should_redraw) {
            tile.tile = m.atlas.get(instance.level.at(static_cast<usize>(safe_index)));
        }
    }

    instance.tiles_need_to_reset = false;
}

auto sprite::Tiles::render(void) noexcept -> void {
    for (auto& tile : m.tiles) {
        tile.tile.render(tile.position - sprite::State::instance().camera_position, tile.transform);
    }
}

auto sprite::Tiles::quit(void) noexcept -> void {
    for (auto& tile : m.tiles) {
        tile.tile.destroy();
    }

    m.atlas.destroy();
}
