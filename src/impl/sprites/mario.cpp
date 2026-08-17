#include "defs/sprites/mario.h"

#include "defs/backend/sprite/tileatlas.h"
#include "defs/backend/state.h"
#include "defs/sprites/level.h"
#include "defs/sprites/settings.h"
#include "defs/sprites/state.h"
#include "defs/types.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

auto sprite::Mario::start(::SDL_Renderer* const renderer) noexcept -> void {
    backend::State& game_instance{ backend::State::instance() };
    sprite::State&  sprite_instance{ sprite::State::instance() };

    m.atlas.create(renderer, MARIO_PATH, MARIO_MAP);
    m.sprite = m.atlas.get("idle");

    const f64 scale{ sprite_instance.settings.get(combine("mario", "scale")) };

    position     = { game_instance.renderer_size.x / -2. + 200, game_instance.renderer_size.y / -2. + 200 };
    transform    = { scale, 0, 0, scale };
    object_layer = 50;

    m.frames_falling = 1000;
    m.frames_jumping = 1000;

    {
        sprite_instance.camera_position.x = -sprite_instance.scaled_tile / 2;
        sprite_instance.camera_position.y = -sprite_instance.scaled_tile / 2;
    }
}

auto sprite::Mario::update(void) noexcept -> void {
    const f64 dt{ backend::State::instance().dt };

    if (sprite::State::instance().is_editing) {
        move_editing_mode();

        position.x += m.velocity.x * dt;
        position.y += m.velocity.y * dt;
    } else {
        move_x_direction();
        move_y_direction();

        position.x += m.velocity.x * dt;
        resolve_x_collisions();

        m.frames_falling++;
        position.y += m.velocity.y * dt;

        resolve_y_collisions();
    }

    set_camera_position();

    set_mario_skin();
}

auto sprite::Mario::render(void) noexcept -> void {
    m.sprite.render(position - sprite::State::instance().camera_position, transform);
}

auto sprite::Mario::quit(void) noexcept -> void {
    m.sprite.destroy();
    m.atlas.destroy();
}

auto sprite::Mario::set_camera_position(void) noexcept -> void {
    sprite::State& instance{ sprite::State::instance() };

    if (instance.is_editing) {
        instance.camera_position.x = position.x;
        instance.camera_position.y = position.y;
    } else {
        instance.camera_position.x = position.x;
        instance.camera_position.y += (position.y - instance.camera_position.y) / instance.settings.get(combine("mario", "tracking-delay"));
    }

    instance.camera_position.x = std::clamp(
        static_cast<f64>(instance.camera_position.x),
        -instance.scaled_tile / 2,
        instance.scaled_tile * instance.level_size.x - backend::State::instance().renderer_size.x - instance.scaled_tile / 2
    );
    instance.camera_position.y = std::clamp(
        static_cast<f64>(instance.camera_position.y),
        -instance.scaled_tile / 2,
        instance.scaled_tile * instance.level_size.y - backend::State::instance().renderer_size.y - instance.scaled_tile / 2
    );
}

auto sprite::Mario::move_editing_mode(void) noexcept -> void {
    backend::State&   instance{ backend::State::instance() };
    sprite::Settings& settings{ sprite::State::instance().settings };

    const i8 x_direction{ static_cast<i8>(
        (instance.keys.contains(SDL_SCANCODE_D) || instance.keys.contains(SDL_SCANCODE_RIGHT))
        - (instance.keys.contains(SDL_SCANCODE_A) || instance.keys.contains(SDL_SCANCODE_LEFT))
    ) };

    const i8 y_direction{ static_cast<i8>(
        (instance.keys.contains(SDL_SCANCODE_W) || instance.keys.contains(SDL_SCANCODE_UP))
        - (instance.keys.contains(SDL_SCANCODE_S) || instance.keys.contains(SDL_SCANCODE_DOWN))
    ) };

    m.velocity.x += x_direction * settings.get(combine("mario", "editing-speed"));
    m.velocity.y += y_direction * settings.get(combine("mario", "editing-speed"));

    m.velocity.x *= settings.get(combine("mario", "editing-friction"));
    m.velocity.y *= settings.get(combine("mario", "editing-friction"));
}

auto sprite::Mario::move_x_direction(void) noexcept -> void {
    backend::State&   instance{ backend::State::instance() };
    sprite::Settings& settings{ sprite::State::instance().settings };

    const i8 direction{ static_cast<i8>(
        (instance.keys.contains(SDL_SCANCODE_D) || instance.keys.contains(SDL_SCANCODE_RIGHT))
        - (instance.keys.contains(SDL_SCANCODE_A) || instance.keys.contains(SDL_SCANCODE_LEFT))
    ) };

    if (direction != 0) {
        // change sprite direction
        transform[0][0] = static_cast<float>(direction * settings.get(combine("mario", "scale")));

        f64 current_acceleration{ settings.get(combine("mario", "acceleration")) };

        if ((direction > 0 && m.velocity.x < 0) || (direction < 0 && m.velocity.x > 0)) {
            current_acceleration *= settings.get(combine("mario", "skid-multiplier"));

            m.special_actions = M::SpecialActions::Skidding;
            transform[0][0] *= -1;
        } else {
            m.special_actions = M::SpecialActions::Normal;
        }

        const f64 speed{ settings.get(combine("mario", "maximum-speed")) };

        m.velocity.x += direction * current_acceleration * instance.dt;
        m.velocity.x = std::clamp(static_cast<f64>(m.velocity.x), -speed, speed);
    } else {
        m.special_actions = M::SpecialActions::Normal;

        if (m.frames_falling < settings.get(combine("mario", "coyote-time"))) {  // do not have friction in the air
            m.velocity.x *= std::pow(settings.get(combine("mario", "slip-factor")), instance.dt * 10);

            if (std::abs(m.velocity.x) < 1.) {
                m.velocity.x = 0;
                m.walk_frame = 0;
            }
        }
    }

    f64 walk_frame_speed{ std::abs(m.velocity.x) / 400 };
    if (std::abs(m.velocity.x) < 10.) {
        walk_frame_speed = 0;
    }

    m.walk_frame += walk_frame_speed;
}

auto sprite::Mario::move_y_direction(void) noexcept -> void {
    backend::State&   instance{ backend::State::instance() };
    sprite::Settings& settings{ sprite::State::instance().settings };

    const f64 gravity{ settings.get(combine("mario", "gravity")) };
    const f64 jump_strength{ settings.get(combine("mario", "jump-strength")) };

    m.velocity.y -= gravity;
    m.velocity.y = std::clamp(static_cast<f64>(m.velocity.y), -gravity * 20, static_cast<f64>(std::numeric_limits<f64>::max()));

    const b8 attempting_to_jump{ instance.keys.contains(SDL_SCANCODE_W) || instance.keys.contains(SDL_SCANCODE_UP) };

    if (attempting_to_jump && m.frames_jumping > 0 && m.frames_jumping < settings.get(combine("mario", "jump-hold-time"))) {  // start of jump
        m.frames_jumping++;
        m.velocity.y = jump_strength;
    } else if (
        attempting_to_jump && m.frames_jumping == 0 && m.frames_falling <= settings.get(combine("mario", "coyote-time")) && m.velocity.y <= 0
    ) {
        m.frames_jumping = 1;
        m.velocity.y     = jump_strength;
    } else {
        m.frames_jumping = 0;
    }
}

auto sprite::Mario::resolve_x_collisions(void) noexcept -> void {
    if (!is_colliding()) {
        return;
    }

    const sprite::State& instance{ sprite::State::instance() };

    const f64 tile_center_offset{ instance.scaled_tile / 2 };
    const f64 half_width{ m.size.x * static_cast<float>(instance.settings.get(combine("mario", "scale"))) / 2.f };
    const f64 window_offset_x{ backend::State::instance().renderer_size.x / 2.f };

    if (m.velocity.x > 0) {
        f64 world_x{ position.x + half_width + window_offset_x + tile_center_offset };
        f64 tile_right_edge{ std::floor(world_x / instance.scaled_tile) * instance.scaled_tile };

        position.x = tile_right_edge - window_offset_x - tile_center_offset - half_width - TINY;
    } else if (m.velocity.x < 0) {
        f64 world_x{ position.x - half_width + window_offset_x + tile_center_offset };
        f64 tile_left_edge{ std::ceil(world_x / instance.scaled_tile) * instance.scaled_tile };

        position.x = tile_left_edge - window_offset_x - tile_center_offset + half_width + TINY;
    }

    m.velocity.x = 0;
}

auto sprite::Mario::resolve_y_collisions(void) noexcept -> void {
    if (!is_colliding()) {
        return;
    }

    const sprite::State&    instance{ sprite::State::instance() };
    const sprite::Settings& settings{ sprite::State::instance().settings };

    const f64 tile_center_offset{ instance.scaled_tile / 2.0 };
    const f64 half_height{ m.size.y * static_cast<float>(settings.get(combine("mario", "scale"))) / 2.f };
    const f64 window_offset_y{ backend::State::instance().renderer_size.y / 2.f };

    if (m.velocity.y < 0) {
        const f64 tile_top_edge{ std::ceil((position.y - half_height + window_offset_y + tile_center_offset) / instance.scaled_tile)
                                 * instance.scaled_tile };

        position.y = tile_top_edge - window_offset_y - tile_center_offset + half_height + TINY;

        // sometimes when jumping mid skid, the sprite may not become normal again
        if (m.frames_falling > settings.get(combine("mario", "coyote-time"))) {
            m.special_actions = M::SpecialActions::Normal;
        }

        // reset falling
        m.frames_falling = 0;
    }

    if (m.velocity.y > 0) {
        const f64 tile_bottom_edge{ std::floor((position.y + half_height + window_offset_y + tile_center_offset) / instance.scaled_tile)
                                    * instance.scaled_tile };

        position.y = tile_bottom_edge - window_offset_y - tile_center_offset - half_height - TINY;

        // we hit our head, we shouldn't keep jumping
        m.frames_jumping = settings.get(combine("mario", "jump-hold-time")) + 100;
    }

    m.velocity.y = 0;
}

auto sprite::Mario::is_colliding(void) const noexcept -> b8 {
    const sprite::State& instance{ sprite::State::instance() };
    const glm::vec2      half_size{ m.size * static_cast<float>(instance.settings.get(combine("mario", "scale"))) / 2.f };

    const std::array points{
        glm::vec2{ position.x - half_size.x, position.y },
        glm::vec2{ position.x - half_size.x, position.y + half_size.y },
        glm::vec2{ position.x - half_size.x, position.y - half_size.y },

        glm::vec2{ position.x + half_size.x, position.y },
        glm::vec2{ position.x + half_size.x, position.y + half_size.y },
        glm::vec2{ position.x + half_size.x, position.y - half_size.y },
    };

    for (const auto& point : points) {
        if (instance.level[sprite::word_coordinate_to_tile_index(point)] != "air") {
            return true;
        }
    }

    return false;
}

auto sprite::Mario::set_mario_skin(void) noexcept -> void {
    if (sprite::State::instance().is_editing) {
        m.sprite = m.atlas.get("idle");

        return;
    }

    if (m.frames_falling > 1) {
        if (m.velocity.y > 0) {
            m.sprite = m.atlas.get("jumping");
        } else {
            m.sprite = m.atlas.get("falling");
        }

        return;
    }

    if (m.special_actions == M::SpecialActions::Skidding) {
        m.sprite = m.atlas.get("skid");

        return;
    }

    if (std::abs(m.velocity.x) > 1) {
        m.sprite = m.atlas.get(std::format("walk-{}", static_cast<usize>(std::floor(m.walk_frame)) % 2));

        return;
    }

    m.sprite = m.atlas.get("idle");
}
