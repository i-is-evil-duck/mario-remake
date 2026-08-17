#pragma once

#include "defs/autorelease.h"
#include "defs/types.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <bitset>
#include <glm/glm.hpp>
#include <unordered_set>

namespace backend {
    class State final {
    public:

        ~State() noexcept;

        State(const backend::State&) noexcept                             = delete;
        auto operator=(const backend::State&) noexcept -> backend::State& = delete;

        State(backend::State&&) noexcept                             = delete;
        auto operator=(backend::State&&) noexcept -> backend::State& = delete;

        static auto instance(void) noexcept -> backend::State&;

        glm::vec2                          window_size{ 1280, 720 };
        glm::vec2                          renderer_size{};
        std::unordered_set<::SDL_Scancode> just_pressed_keys{};
        std::unordered_set<::SDL_Scancode> keys{};
        ::SDL_Keymod                       just_pressed_mods{};
        ::SDL_Keymod                       mods{};
        glm::vec2                          mouse_position{};
        std::bitset<3>                     mouse_state{};
        AutoRelease<::TTF_TextEngine>      text_engine{};
        AutoRelease<::TTF_Font>            font{};

        f64 dt{};

    private:

        State(void) noexcept;
    };
}
