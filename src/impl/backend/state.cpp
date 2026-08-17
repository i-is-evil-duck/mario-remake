#include "defs/backend/state.h"

backend::State::State() noexcept  = default;
backend::State::~State() noexcept = default;

auto backend::State::instance(void) noexcept -> backend::State& {
    static backend::State instance{};

    return instance;
}
