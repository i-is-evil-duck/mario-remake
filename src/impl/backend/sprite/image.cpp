#include "defs/backend/sprite/image.h"

#include "defs/backend/sprite/render.h"

#include <SDL3_image/SDL_image.h>
#include <telemetry/telemetry.hpp>

backend::sprite::Image::Image(void) noexcept = default;

backend::sprite::Image::Image(const backend::sprite::Image& other) noexcept {
    telemetry::trace("copying image");

    create(other.m.renderer, other.m.path);
}

auto backend::sprite::Image::operator=(const backend::sprite::Image& other) noexcept -> backend::sprite::Image& {
    if (this == &other) {
        telemetry::datum("copying image to itself");

        return *this;
    }

    telemetry::trace("copying image");

    create(other.m.renderer, other.m.path);

    return *this;
}

backend::sprite::Image::Image(backend::sprite::Image&& other) noexcept {
    m = std::move(other.m);

    other.m.initialized = false;
}

auto backend::sprite::Image::operator=(sprite::Image&& other) noexcept -> backend::sprite::Image& {
    m = std::move(other.m);

    other.m.initialized = false;

    return *this;
}

backend::sprite::Image::~Image() noexcept {
    if (!m.initialized) [[likely]] {
        return;
    }

    telemetry::alert("image not destroyed manually");
    telemetry::datum("destroying image automatically");

    destroy();
}

auto backend::sprite::Image::create(::SDL_Renderer* const renderer, const std::filesystem::path& path) noexcept -> void {
    telemetry::validate(!m.initialized, "image already initialized");

    m.path     = path;
    m.renderer = renderer;

    const auto string_path = path.string();

    telemetry::trace("creating image at: {}", string_path);

    // create image
    m.texture = AutoRelease<::SDL_Texture>{ ::IMG_LoadTexture(renderer, string_path.c_str()), ::SDL_DestroyTexture };
    telemetry::validate(m.texture.data != nullptr, "failed to make image: {}", ::SDL_GetError());
    telemetry::validate(::SDL_SetTextureScaleMode(m.texture.data, SDL_SCALEMODE_NEAREST), "failed to set texture mode");

    // set info about image
    f32 width{}, height{};
    telemetry::validate(::SDL_GetTextureSize(m.texture.data, &width, &height), "failed to get image size");
    m.size = glm::vec2{ width, height };
    telemetry::trace("image size is ({}, {})", m.size.x, m.size.y);

    telemetry::trace("created image");
    m.initialized = true;
}

auto backend::sprite::Image::size() const noexcept -> glm::vec2 {
    return m.size;
}

auto backend::sprite::Image::destroy(void) noexcept -> void {
    telemetry::validate(m.initialized, "image not initialized or already destroyed");

    m.texture.destroy();

    m.initialized = false;
}

auto backend::sprite::Image::get(void) const noexcept -> ::SDL_Texture* {
    return m.texture.data;
}

auto backend::sprite::Image::render(const glm::vec2& position, const glm::mat2x2& transform) const noexcept -> void {
    telemetry::validate(m.initialized, "rendering a nonexistent image");

    constexpr std::array uv_coordinates{
        ::SDL_FPoint{ .x = 0, .y = 0 }, // top left
        ::SDL_FPoint{ .x = 1, .y = 0 }, // top right
        ::SDL_FPoint{ .x = 0, .y = 1 }, // bottom left
        ::SDL_FPoint{ .x = 1, .y = 1 }  // bottom right
    };

    constexpr std::array        indicies{ 0, 1, 2, 2, 1, 3 };
    std::array<::SDL_Vertex, 4> vertices{ backend::sprite::get_image_draw_command(position, transform, m.size, uv_coordinates) };

    ::SDL_RenderGeometry(m.renderer, m.texture.data, vertices.data(), std::size(vertices), indicies.data(), std::size(indicies));
}
