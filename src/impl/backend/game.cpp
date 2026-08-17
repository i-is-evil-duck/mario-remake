#include "defs/backend/game.h"

#include "defs/loop.h"

#include <algorithm>
#include <telemetry/telemetry.hpp>

backend::Game::Game(void) noexcept = default;

backend::Game::~Game(void) noexcept {
    if (!m.initialized) [[likely]] {
        return;
    }

    telemetry::alert("game manager not destroyed manually");
    telemetry::datum("destroying game manager automatically");

    destroy();
}

auto backend::Game::create(::SDL_Renderer* const renderer) noexcept -> void {
    telemetry::validate(!m.initialized, "game manager already initialized");

    loop::start(renderer, m.entities);

    for (auto& entity : m.entities) {
        entity->start(renderer);
    }

    telemetry::trace("created game manager");
    m.initialized = true;
}

auto backend::Game::destroy(void) noexcept -> void {
    telemetry::validate(m.initialized, "game manager not initialized or already destroyed");

    loop::quit();

    for (auto& entity : m.entities) {
        entity->quit();
    }

    m.initialized = false;
}

auto backend::Game::tick(void) noexcept -> void {
    loop::update();

    for (auto& entity : m.entities) {
        entity->update();
    }

    m.layer_sorted_entities.clear();

    for (usize i{}; i < m.entities.size(); ++i) {
        if (!m.entities[i]->hidden) [[unlikely]] {
            m.layer_sorted_entities.emplace_back(std::make_pair(m.entities[i]->object_layer, i));
        }
    }

    std::sort(m.layer_sorted_entities.begin(), m.layer_sorted_entities.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    for (auto [_, entity_index] : m.layer_sorted_entities) {
        m.entities[entity_index]->render();
    }
}
