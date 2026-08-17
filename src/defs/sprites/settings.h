#pragma once

#include "defs/types.h"

#include <string_view>
#include <unordered_map>

namespace sprite {
    class Settings {
    public:

        Settings() noexcept;
        ~Settings() noexcept;

        Settings(const sprite::Settings&) noexcept                            = delete;
        auto operator=(const sprite::Settings&) noexcept -> sprite::Settings& = delete;

        Settings(sprite::Settings&&) noexcept                            = delete;
        auto operator=(sprite::Settings&&) noexcept -> sprite::Settings& = delete;

        /**
         * @brief Setup the settings of the game
         */
        auto set_settings() noexcept -> void;

        /**
         * @brief Get a setting from a hashed key
         *
         * @param hash Hash from `combine`
         *
         * @returns Setting
         *
         * @see combine
         */
        auto get(const u64 hash) const noexcept -> f64;

    private:

        struct M {
            std::unordered_map<u64, f64> settings{};
        } m;

        static constexpr std::string_view SETTINGS_PATH{ "res/settings.toml" };
    };
}

/**
 * @brief Combine two constant strings in compile time for getting settings in runtime
 *
 * @param section Section to get key from
 * @param key Key to get from section
 *
 * @returns Hash to be used in get function
 */
constexpr auto combine(std::string_view section, std::string_view key) -> u64 {
    u64 hash = 0xcbf29ce484222325;

    for (char c : section) {
        hash ^= static_cast<u8>(c);
        hash *= 0x100000001b3;
    }

    hash ^= static_cast<u8>(':');
    hash *= 0x100000001b3;
    hash ^= static_cast<u8>(':');
    hash *= 0x100000001b3;

    for (char c : key) {
        hash ^= static_cast<u8>(c);
        hash *= 0x100000001b3;
    }

    return hash;
}
