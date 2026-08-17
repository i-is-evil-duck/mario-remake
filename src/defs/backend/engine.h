#pragma once

#include "defs/autorelease.h"
#include "defs/types.h"

#include <SDL3/SDL_render.h>
#include <filesystem>
#include <string_view>

struct TTF_Text;

namespace backend {
    class Game;
    class DeltaTime;

    class Engine final {
    public:

        Engine(void) noexcept;
        ~Engine() noexcept;

        Engine(const backend::Engine&) noexcept                             = delete;
        auto operator=(const backend::Engine&) noexcept -> backend::Engine& = delete;

        Engine(backend::Engine&&) noexcept                             = delete;
        auto operator=(backend::Engine&&) noexcept -> backend::Engine& = delete;

        /**
         * @brief Create engine
         */
        auto create(void) noexcept -> void;

        /**
         * @brief Run engine
         */
        auto run(void) noexcept -> void;

        /**
         * @brief Destroy engine
         */
        auto destroy(void) noexcept -> void;

        /**
         * @brief Process one frame (used by Emscripten main loop)
         */
        auto tick(void) noexcept -> void;

    private:

        /**
         * @brief Process inputs given to window
         */
        auto process_inputs(void) noexcept -> void;

    private:

        struct M {
            b8 initialized{};

            AutoRelease<::SDL_Window>   window{};
            AutoRelease<::SDL_Renderer> renderer{};
            b8                          running{};
        } m;

        const std::string_view      TITLE{ "Mario Game" };
        const std::filesystem::path ICON_PATH{ "res/images/logo.png" };
        const std::filesystem::path FONT_PATH{ "res/fonts/SMW.ttf" };

        // Loop state (needed for Emscripten main loop callback)
        struct LoopState {
            f64 accumulated_time{};
            backend::Game*     game_manager{};
            backend::DeltaTime* delta_time_manager{};
            AutoRelease<::TTF_Text> text{};
            bool initialized{};
        } loop{};

#ifdef __EMSCRIPTEN__
        static auto em_loop_callback(void* arg) noexcept -> void;
#endif
    };
}
