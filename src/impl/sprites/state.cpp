#include "defs/sprites/state.h"

sprite::State::State() noexcept  = default;
sprite::State::~State() noexcept = default;

auto sprite::State::instance(void) noexcept -> sprite::State& {
    static sprite::State instance{};

    return instance;
}
