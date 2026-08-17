#include "defs/backend/engine.h"

#include "defs/autorelease.h"
#include "defs/backend/deltatime.h"
#include "defs/backend/game.h"
#include "defs/backend/sprite/render.h"
#include "defs/backend/state.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <telemetry/telemetry.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

backend::Engine::Engine() noexcept = default;

backend::Engine::~Engine() noexcept {
    if (!m.initialized) [[likely]] {
        return;
    }

    telemetry::alert("engine not destroyed manually");
    telemetry::datum("destroying engine automatically");

    destroy();
}

auto backend::Engine::create(void) noexcept -> void {
    telemetry::validate(!m.initialized, "engine already initialized");
    telemetry::trace("initializing engine");

    backend::State& instance{ backend::State::instance() };

    telemetry::validate(::SDL_Init(SDL_INIT_VIDEO), "failed initializing SDL3: {}", ::SDL_GetError());
    telemetry::validate(::TTF_Init(), "failed initializing SDL3-ttf: {}", ::SDL_GetError());

    ::SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    ::SDL_Window*   window{};
    ::SDL_Renderer* renderer{};

    telemetry::validate(
        ::SDL_CreateWindowAndRenderer(
            TITLE.data(), instance.window_size.x, instance.window_size.y, SDL_WINDOW_HIGH_PIXEL_DENSITY, &window, &renderer
        ),
        "failed to create window/renderer: {}",
        ::SDL_GetError()
    );

    m.window   = AutoRelease<::SDL_Window>{ window, ::SDL_DestroyWindow };
    m.renderer = AutoRelease<::SDL_Renderer>{ renderer, ::SDL_DestroyRenderer };
    window     = nullptr;
    renderer   = nullptr;

    telemetry::validate(::SDL_SetRenderDrawColor(m.renderer.data, 255, 0, 255, 0), "failed setting render clear colour: {}", ::SDL_GetError());
    telemetry::debug("using renderer backend of: {}", ::SDL_GetRendererName(m.renderer.data));

    instance.font = { ::TTF_OpenFont(FONT_PATH.string().c_str(), 24.0f), ::TTF_CloseFont };
    telemetry::validate(instance.font.data != nullptr, "failed to load font: {}", ::SDL_GetError());
    instance.text_engine = { ::TTF_CreateRendererTextEngine(m.renderer.data), ::TTF_DestroyRendererTextEngine };
    telemetry::validate(instance.text_engine.data != nullptr, "failed to create text engine: {}", ::SDL_GetError());

    telemetry::trace("initialized engine");
    m.initialized = true;
}

auto backend::Engine::destroy(void) noexcept -> void {
    telemetry::validate(m.initialized, "engine not initialized or already destroyed");

    backend::State::instance().font.destroy();
    backend::State::instance().text_engine.destroy();
    ::TTF_Quit();
    ::SDL_Quit();

    telemetry::trace("destroyed engine");
    m.initialized = false;
}

auto backend::Engine::run(void) noexcept -> void {
    backend::State& instance{ backend::State::instance() };

    i32 renderer_width{}, renderer_height{};
    ::SDL_GetCurrentRenderOutputSize(m.renderer.data, &renderer_width, &renderer_height);

    instance.renderer_size = { renderer_width, renderer_height };

    i32 window_width{}, window_height{};
    ::SDL_GetWindowSize(m.window.data, &window_width, &window_height);

    instance.window_size = { window_width, window_height };

    loop.game_manager = new backend::Game{};
    loop.game_manager->create(m.renderer.data);

    loop.delta_time_manager = new backend::DeltaTime{};
    loop.delta_time_manager->create();

    loop.text = { ::TTF_CreateText(instance.text_engine.data, instance.font.data, "FPS 0", 0), ::TTF_DestroyText };

    m.running = true;

    ::SDL_RaiseWindow(m.window.data);
    ::SDL_SetWindowPosition(m.window.data, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    loop.initialized = true;

#ifdef __EMSCRIPTEN__
    ::emscripten_set_main_loop_arg(em_loop_callback, this, 0, 1);
#else
    while (m.running) {
        tick();
    }
#endif
}

auto backend::Engine::tick(void) noexcept -> void {
    backend::State& instance{ backend::State::instance() };

    process_inputs();

    SDL_RenderClear(m.renderer.data);

    instance.dt = loop.delta_time_manager->get();

    loop.accumulated_time += instance.dt;

    if (loop.accumulated_time >= 0.1) {
        ::TTF_SetTextString(loop.text.data, std::format("FPS: {:06.0f}", 1 / instance.dt).c_str(), 0);

        loop.accumulated_time = 0;
    }

    loop.game_manager->tick();
    ::TTF_DrawRendererText(loop.text.data, 5, 5);

    ::SDL_RenderPresent(m.renderer.data);

#ifndef __EMSCRIPTEN__
    if (!m.running) {
        loop.delta_time_manager->destroy();
        loop.game_manager->destroy();
        delete loop.game_manager;
        delete loop.delta_time_manager;
        loop.initialized = false;
    }
#endif
}

#ifdef __EMSCRIPTEN__
auto backend::Engine::em_loop_callback(void* arg) noexcept -> void {
    auto* engine = static_cast<backend::Engine*>(arg);

    if (!engine->m.running) {
        engine->loop.delta_time_manager->destroy();
        engine->loop.game_manager->destroy();
        delete engine->loop.game_manager;
        delete engine->loop.delta_time_manager;
        engine->loop.initialized = false;
        ::emscripten_cancel_main_loop();
        return;
    }

    engine->tick();
}
#endif

auto backend::Engine::process_inputs(void) noexcept -> void {
    backend::State& instance{ backend::State::instance() };

    ::SDL_Event event{};

    instance.keys.merge(instance.just_pressed_keys);

    instance.mods |= instance.just_pressed_mods;
    instance.just_pressed_mods = 0;

    while (::SDL_PollEvent(&event)) {
        ::SDL_ConvertEventToRenderCoordinates(m.renderer.data, &event);

        switch (event.type) {
        case SDL_EVENT_QUIT: {
            m.running = false;

            break;
        }

        case SDL_EVENT_KEY_DOWN: {
            if (!event.key.repeat) {
                instance.just_pressed_keys.emplace(event.key.scancode);
                instance.just_pressed_mods = event.key.mod;
            }

            break;
        }

        case SDL_EVENT_KEY_UP: {
            instance.keys.erase(event.key.scancode);
            instance.mods = event.key.mod;

            break;
        }

        case SDL_EVENT_WINDOW_RESIZED: {
            i32 renderer_width{}, renderer_height{};
            ::SDL_GetCurrentRenderOutputSize(m.renderer.data, &renderer_width, &renderer_height);

            instance.renderer_size = { renderer_width, renderer_height };

            i32 window_width{}, window_height{};

            ::SDL_GetWindowSize(m.window.data, &window_width, &window_height);
            instance.window_size = { window_width, window_height };

            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            instance.mouse_position = backend::sprite::sdl3_to_cartesian_coordinates({ event.motion.x, event.motion.y }, instance.renderer_size);

            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            instance.mouse_state.set(event.button.button - 1, true);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            instance.mouse_state.set(event.button.button - 1, false);
            break;
        }

        default: break;
        }
    }
}
