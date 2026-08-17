#include "defs/backend/deltatime.h"

#include <SDL3/SDL.h>
#include <telemetry/telemetry.hpp>

backend::DeltaTime::DeltaTime(void) noexcept = default;

backend::DeltaTime::~DeltaTime() noexcept {
    if (!m.initialized) [[likely]] {
        return;
    }

    telemetry::alert("delta-time manager not destroyed manually");
    telemetry::datum("destroying delta-time manager automatically");

    destroy();
}

auto backend::DeltaTime::create(void) noexcept -> void {
    telemetry::validate(!m.initialized, "delta-time manager already initialized");
    telemetry::trace("initializing delta-time manager");

    m.last = SDL_GetTicksNS();

    telemetry::trace("initialized delta-time manager");
    m.initialized = true;
}

auto backend::DeltaTime::destroy(void) noexcept -> void {
    telemetry::validate(m.initialized, "delta-time manager not initialized or already destroyed");

    telemetry::trace("destroyed delta-time manager");
    m.initialized = false;
}

auto backend::DeltaTime::get(void) noexcept -> f64 {
    const u64 now{ SDL_GetTicksNS() };
    const f64 dt{ (now - m.last) / 1'000'000'000. };

    m.last = now;

    return std::min(dt, 1.0 / 30.0);
}
