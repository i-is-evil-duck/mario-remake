#include "defs/sprites/settings.h"

#include "defs/toml.h"
#include "telemetry/telemetry.hpp"

#include <sstream>
#include <string_view>

sprite::Settings::Settings() noexcept  = default;
sprite::Settings::~Settings() noexcept = default;

auto sprite::Settings::set_settings() noexcept -> void {
    const toml::table table{ toml::deserialize(SETTINGS_PATH) };

    for (const auto& sections : table) {
        const std::string_view section{ sections.first };

        for (const auto& item : *sections.second.as_table()) {
            const std::string_view key{ item.first };

            if (item.second.is_floating_point()) {
                m.settings.emplace(combine(section, key), item.second.as<f64>()->get());
            } else if (item.second.is_integer()) {
                m.settings.emplace(combine(section, key), item.second.as<i64>()->get());
            } else {
                std::stringstream ss{};
                ss << item.second.type();

                telemetry::fatal("Invalid type of object at {}:{} of {}", section, key, ss.str());
            }
        }
    }
}

auto sprite::Settings::get(const u64 hash) const noexcept -> f64 {
    return m.settings.at(hash);
}
