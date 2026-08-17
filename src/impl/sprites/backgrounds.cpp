#include "defs/sprites/backgrounds.h"

#include "defs/backend/state.h"
#include "defs/sprites/settings.h"
#include "defs/sprites/state.h"

#include <cmath>

auto sprite::Backgrounds::start(::SDL_Renderer* const renderer) noexcept -> void {
    const sprite::State& instance{ sprite::State::instance() };

    object_layer = 0;

    m.atlas.create(renderer, BACKGROUNDS_PATH, BACKGROUNDS_MAP);

    const f64       scale{ instance.settings.get(combine("background", "scale")) };
    const f32       scaled_tile{ static_cast<f32>(SIZE * scale) };
    const glm::vec2 renderer_size{ backend::State::instance().renderer_size };

    m.grid_size   = renderer_size / scaled_tile + glm::vec2{ 1 };
    m.grid_size.x = std::ceil(m.grid_size.x);
    m.grid_size.y = std::ceil(m.grid_size.y);

    const f64 x_start{ -((m.grid_size.x - 1) * scaled_tile) / 2. };
    m.y_start = renderer_size.y / -2. + scaled_tile / 2. + sprite::State::instance().camera_position.y / 2.;

    for (i64 x{}; x < m.grid_size.x; ++x) {
        for (i64 y{}; y < m.grid_size.y; ++y) {
            m.backgrounds.emplace_back(
                m.atlas.get(y == 0 ? "day-ground" : "day-sky"),
                glm::vec2{ x_start + (x * scaled_tile), m.y_start + (y * scaled_tile) },
                glm::mat2x2{ scale, 0, 0, scale }
            );
        }
    }
}

auto sprite::Backgrounds::update(void) noexcept -> void {
    const sprite::State& instance{ sprite::State::instance() };

    const glm::vec2 renderer_size{ backend::State::instance().renderer_size };
    const u64       scale{ static_cast<u64>(instance.settings.get(combine("background", "scale"))) };
    const f32       speed{ static_cast<f32>(instance.settings.get(combine("background", "speed"))) };

    const u64 scaled_tile{ SIZE * scale };
    const f64 total_width{ m.grid_size.x * scaled_tile };
    const f64 total_height{ m.grid_size.y * scaled_tile };

    for (auto& background : m.backgrounds) {
        const f64 relative_x{ background.position.x - instance.camera_position.x * speed };
        const f64 relative_y{ background.position.y - instance.camera_position.y * speed };

        b8 should_update{};

        if (relative_x < -total_width / 2.) {
            background.position.x += total_width;
        } else if (relative_x > total_width / 2.) {
            background.position.x -= total_width;
        }

        if (relative_y < -total_height / 2.) {
            background.position.y += total_height;

            should_update = true;
        } else if (relative_y > total_height / 2.) {
            background.position.y -= total_height;

            should_update = true;
        }

        if (should_update) {
            if (background.position.y == m.y_start) {
                background.tile = m.atlas.get("day-ground");
            } else {
                background.tile = m.atlas.get("day-sky");
            }
        }
    }
}

auto sprite::Backgrounds::render(void) noexcept -> void {
    const sprite::State& instance{ sprite::State::instance() };

    const f32 speed{ static_cast<f32>(instance.settings.get(combine("background", "speed"))) };

    for (auto& background : m.backgrounds) {
        background.tile.render(background.position - instance.camera_position * speed, background.transform);
    }
}

auto sprite::Backgrounds::quit(void) noexcept -> void {
    for (auto& background : m.backgrounds) {
        background.tile.destroy();
    }

    m.atlas.destroy();
}
