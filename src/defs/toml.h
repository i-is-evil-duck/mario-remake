#pragma once

#include <filesystem>
#include <toml++/toml.hpp>

namespace toml {
    /**
    * @brief Parse a TOML file
    *
    * @param path Path to TOML file
    *
    * @return Parsed TOML
    */
    auto deserialize(const std::filesystem::path& path) noexcept -> toml::table;
}
