#include "defs/toml.h"

#include "defs/types.h"

#include <telemetry/telemetry.hpp>

auto toml::deserialize(const std::filesystem::path& path) noexcept -> toml::table {
    const toml::parse_result result{ toml::parse_file(path.string().c_str()) };

    if (result.succeeded()) [[likely]] {
        return std::move(result).table();
    }

    const toml::parse_error error{ result.error() };
    const u32               line_number{ error.source().begin.line };
    const u32               line_number_digits{ static_cast<u32>(std::log10(line_number + (line_number == 0))) + 1 };
    const u32               column_start{ error.source().begin.column };
    const u32               column_end{ error.source().end.column };

    // can't show an error spanning multiple lines, just show rudimentary result
    telemetry::validate(error.source().begin.line == error.source().end.line, "failed to parse TOML document: {}", error.description());

    // get the line of the TOML document with error
    std::string line{};
    {
        std::ifstream file{ path };
        usize         current_line{};

        while (std::getline(file, line)) {
            if (++current_line == line_number) {
                break;
            }
        }
    }

    std::cout
        << std::format("{} {}:{}:{}]", ::__prefix_to_string(::__LogPrefix::FATAL), *error.source().path, line_number, column_start)
        << ::__prefixes::RESET
        << '\n'
        << std::format("{:>{}}|", "", line_number_digits + 1)
        << '\n'
        << std::format("{} | {}", line_number, line)
        << '\n'
        << std::format("{:>{}}|{:>{}}{}", "", line_number_digits + 1, "", column_start, std::string(column_end - column_start, '^'))
        << '\n'
        << error.description()
        << std::endl;

    std::abort();
}
