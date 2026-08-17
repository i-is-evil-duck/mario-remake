#include "defs/sprites/level.h"

#include "defs/random.h"
#include "defs/sprites/state.h"

#include <format>
#include <ranges>
#include <vector>

auto sprite::generate_level() -> void {
    sprite::State& instance{ sprite::State::instance() };

    instance.level.clear();
    instance.level.reserve(instance.level_size.x * instance.level_size.y);

    for ([[maybe_unused]] auto _ : std::views::iota(0, instance.level_size.y)) {
        instance.level.emplace_back("unbreakable");
    }

    for ([[maybe_unused]] auto _ : std::views::iota(0, instance.level_size.x - 2)) {
        instance.level.emplace_back(std::format("ground-00011111-{}", pseudorandom::range(0, 5)));

        for ([[maybe_unused]] auto _ : std::views::iota(0, instance.level_size.y - 2)) {
            instance.level.emplace_back("air");
        }

        instance.level.emplace_back("unbreakable");
    }

    for ([[maybe_unused]] auto _ : std::views::iota(0, instance.level_size.y)) {
        instance.level.emplace_back("unbreakable");
    }
}

auto sprite::word_coordinate_to_tile_index(const glm::vec2 position) noexcept -> usize {
    sprite::State& instance{ sprite::State::instance() };

    const f64 x_origin = -(static_cast<f64>(instance.grid_size.x - 1) * instance.scaled_tile) / 2.;
    const f64 y_origin = -(static_cast<f64>(instance.grid_size.y - 1) * instance.scaled_tile) / 2.;

    i64 tile_x = static_cast<i64>(std::round((position.x - x_origin) / instance.scaled_tile));
    i64 tile_y = static_cast<i64>(std::round((position.y - y_origin) / instance.scaled_tile));

    tile_x = std::clamp(tile_x, 0LL, static_cast<i64>(instance.level_size.x - 1));
    tile_y = std::clamp(tile_y, 0LL, static_cast<i64>(instance.level_size.y - 1));

    return static_cast<usize>(tile_x * sprite::State::instance().level_size.y + tile_y);
}
