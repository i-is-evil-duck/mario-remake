#include "defs/loop.h"

#include "defs/sprites/backgrounds.h"
#include "defs/sprites/editor.h"
#include "defs/sprites/level.h"
#include "defs/sprites/mario.h"
#include "defs/sprites/state.h"
#include "defs/sprites/tiles.h"

auto loop::start(::SDL_Renderer* const renderer, std::vector<std::unique_ptr<backend::Entity>>& entities) noexcept -> void {
    ::sprite::generate_level();
    sprite::State::instance().settings.set_settings();

    entities.emplace_back(std::make_unique<::sprite::Tiles>());
    entities.emplace_back(std::make_unique<::sprite::Editor>());
    entities.emplace_back(std::make_unique<::sprite::Mario>());
    entities.emplace_back(std::make_unique<::sprite::Backgrounds>());
}

auto loop::update(void) noexcept -> void { }

auto loop::quit(void) noexcept -> void { }
