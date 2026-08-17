#pragma once

#include "defs/types.h"

#include <telemetry/telemetry.hpp>
#include <utility>

template <typename T> class AutoRelease {
    using Deleter = void (*)(T*);

public:

    AutoRelease(void) noexcept = default;

    AutoRelease(T* data) noexcept {
        this->data = data;

        m.initialized = true;
    }

    AutoRelease(T* data, Deleter release_function) noexcept {
        this->data = data;

        m.release_function = release_function;
        m.initialized      = true;
    }

    ~AutoRelease() noexcept {
        if (m.initialized) {
            destroy();
        }
    }

    AutoRelease(const AutoRelease&) noexcept                    = delete;
    auto operator=(const AutoRelease&) noexcept -> AutoRelease& = delete;

    AutoRelease(AutoRelease&& other) noexcept {
        data = other.data;
        m    = std::move(other.m);

        other.m.initialized = false;
    }

    auto operator=(AutoRelease&& other) noexcept -> AutoRelease& {
        data = other.data;
        m    = std::move(other.m);

        other.m.initialized = false;

        return *this;
    }

    auto destroy(void) noexcept -> void {
        telemetry::validate(m.initialized && data != nullptr, "Attempted of an object that is destroyed or not initialized yet");

        m.release_function(data);
        data = nullptr;

        m.initialized = false;
    }

    T* data{};

private:

    struct M {
        b8      initialized{};
        Deleter release_function{};
    } m;
};
