#include "defs/backend/sprite/tileatlas.h"

#include "defs/backend/sprite/render.h"
#include "defs/toml.h"

using namespace std::string_view_literals;

backend::sprite::TileAtlas::TileAtlas(void) noexcept = default;

backend::sprite::TileAtlas::~TileAtlas() noexcept {
    if (!m.initialized) [[likely]] {
        return;
    }

    telemetry::alert("tile map not destroyed manually");
    telemetry::datum("destroying tile map automatically");

    destroy();
}

auto backend::sprite::TileAtlas::create(
    ::SDL_Renderer* const renderer, const std::filesystem::path& map_path, const std::filesystem::path& configuration_path
) noexcept -> void {
    telemetry::validate(!m.initialized, "tile map already initialized");

    m.renderer = renderer;

    // create image
    backend::sprite::Image image{};
    image.create(m.renderer, map_path);
    m.texture = std::move(image);

    // set configuration
    const toml::table toml_configuration{ toml::deserialize(configuration_path) };

    const usize default_width{ static_cast<usize>(toml_configuration["default-width"].value_or(1)) };
    const usize default_height{ static_cast<usize>(toml_configuration["default-height"].value_or(1)) };

    for (const auto& [key, value] : toml_configuration) {
        if (value.type() != toml::node_type::table) {
            continue;
        }

        const toml::table& data{ *value.as_table() };

        const sprite::TileAtlas::TileRect result{
            .position = glm::vec2{ data["x"].value_or(0.), data["y"].value_or(0.) },
            .size     = glm::vec2{ data["width"].value_or(default_width), data["height"].value_or(default_height) },
        };

        m.configurations.emplace(key, result);
    }

    telemetry::trace("created tile map from {}", configuration_path.string());

    m.initialized = true;
}

auto backend::sprite::TileAtlas::destroy(void) noexcept -> void {
    telemetry::validate(m.initialized, "image not initialized or already destroyed");

    m.texture.destroy();
    m.initialized = false;
}

auto backend::sprite::TileAtlas::get(const std::string_view& name) noexcept -> sprite::TileAtlas::Tile {
    const std::string key{ name };
    if (m.configurations.find(key) == m.configurations.end()) [[unlikely]] {
        telemetry::fatal("'{}' not found in tile atlas", name);
    }

    const sprite::TileAtlas::TileRect rect{ m.configurations.at(key) };

    const std::array configuration{
        ::SDL_FPoint{
            .x = rect.position.x / m.texture.size().x,
            .y = rect.position.y / m.texture.size().y,
        },
        ::SDL_FPoint{
            .x = (rect.position.x + rect.size.x) / m.texture.size().x,
            .y = rect.position.y / m.texture.size().y,
        },
        ::SDL_FPoint{
            .x = rect.position.x / m.texture.size().x,
            .y = (rect.position.y + rect.size.y) / m.texture.size().y,
        },
        ::SDL_FPoint{
            .x = (rect.position.x + rect.size.x) / m.texture.size().x,
            .y = (rect.position.y + rect.size.y) / m.texture.size().y,
        },
    };

    sprite::TileAtlas::Tile tile{};
    tile.create(m.renderer, m.texture.get(), rect.size, std::move(configuration));

    return tile;
}

backend::sprite::TileAtlas::Tile::Tile(void) noexcept = default;

backend::sprite::TileAtlas::Tile::~Tile() noexcept {
    if (!m.initialized) [[likely]] {
        return;
    }

    telemetry::alert("tile not destroyed manually");
    telemetry::datum("destroying tile automatically");

    destroy();
}

backend::sprite::TileAtlas::Tile::Tile(const backend::sprite::TileAtlas::Tile&) noexcept = default;
auto backend::sprite::TileAtlas::Tile::operator=(const backend::sprite::TileAtlas::Tile&) noexcept
    -> backend::sprite::TileAtlas::Tile& = default;

backend::sprite::TileAtlas::Tile::Tile(backend::sprite::TileAtlas::Tile&& other) noexcept {
    m = std::move(other.m);

    other.m.initialized = false;
}

auto backend::sprite::TileAtlas::Tile::operator=(backend::sprite::TileAtlas::Tile&& other) noexcept -> backend::sprite::TileAtlas::Tile& {
    m = std::move(other.m);

    other.m.initialized = false;

    return *this;
}

auto backend::sprite::TileAtlas::Tile::create(
    ::SDL_Renderer* const renderer, ::SDL_Texture* const texture, const glm::vec2& size, const std::array<::SDL_FPoint, 4>&& uv_coordinates
) noexcept -> void {
    telemetry::validate(!m.initialized, "tile already initialized");

    m.renderer       = renderer;
    m.texture        = texture;
    m.size           = size;
    m.uv_coordinates = uv_coordinates;

    m.initialized = true;
}

auto backend::sprite::TileAtlas::Tile::destroy(void) noexcept -> void {
    telemetry::validate(m.initialized, "tile not initialized or already destroyed");

    m.initialized = false;
}

auto backend::sprite::TileAtlas::Tile::render(const glm::vec2& position, const glm::mat2x2& transform, f32 transparency) noexcept -> void {
    auto vertices{ backend::sprite::get_image_draw_command(position, transform, m.size, m.uv_coordinates) };

    if (transparency != 1) [[unlikely]] {
        for (auto& vertex : vertices) {
            vertex.color.a = transparency;
        }
    }

    constexpr int indices[]{ 0, 1, 2, 3, 2, 1 };

    ::SDL_RenderGeometry(m.renderer, m.texture, vertices.data(), std::size(vertices), indices, std::size(indices));
}

auto backend::sprite::TileAtlas::Tile::size(void) const noexcept -> glm::vec2 {
    return m.size;
}
