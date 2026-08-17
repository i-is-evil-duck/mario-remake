#pragma once

#include <random>
#include <type_traits>

namespace pseudorandom {
    /**
     * @brief Pick a number between `min` and `max`, inclusive
     *
     * @param T Integer type
     *
     * @param min Minumum number
     * @param max Maximum number
     *
     * @returns A number in [min, max]
     */
    template <typename T> requires std::is_integral_v<std::remove_cvref_t<T>> auto range(const T min, const T max) noexcept -> T {
        static std::mt19937 gen{ std::random_device{}() };

        std::uniform_int_distribution<T> dist{ min, max };

        return dist(gen);
    }
}
