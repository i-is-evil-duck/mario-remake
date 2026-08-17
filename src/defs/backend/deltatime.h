#pragma once

#include "defs/types.h"

namespace backend {
    class DeltaTime final {
    public:

        DeltaTime(void) noexcept;
        ~DeltaTime() noexcept;

        DeltaTime(const backend::DeltaTime&) noexcept                             = delete;
        auto operator=(const backend::DeltaTime&) noexcept -> backend::DeltaTime& = delete;

        DeltaTime(backend::DeltaTime&&) noexcept                             = delete;
        auto operator=(backend::DeltaTime&&) noexcept -> backend::DeltaTime& = delete;

        /**
         * @brief Create delta time manager
         */
        auto create(void) noexcept -> void;

        /**
         * @brief Destroy delta time manager
         */
        auto destroy(void) noexcept -> void;

        /**
         * @brief Get the time to compute previous frame
         *
         * @attention Should be called each frame
         *
         * @return delta time
         */
        auto get(void) noexcept -> f64;

    private:

        struct M {
            b8 initialized{};

            u64 last{};
        } m;
    };
}
