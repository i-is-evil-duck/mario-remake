#include "defs/sprites/editor.h"

#include "defs/backend/state.h"
#include "defs/random.h"
#include "defs/sprites/level.h"
#include "defs/sprites/settings.h"
#include "defs/sprites/state.h"
#include "defs/toml.h"
#include "defs/types.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <string_view>
#include <telemetry/telemetry.hpp>
#include <utility>
#include <vector>
#include <zstd.h>

auto sprite::Editor::start(::SDL_Renderer* const renderer) noexcept -> void {
    const sprite::State& instance{ sprite::State::instance() };

    const f64 tile_scale{ instance.settings.get(combine("tile", "scale")) };

    transform    = { tile_scale, 0, 0, tile_scale };
    object_layer = 999;

    m.tiles.create(renderer, instance.TILES_PATH, instance.TILES_MAP);

    load_block_data();

    m.selected_tile = m.tile_groups[0];
    m.brush_tile    = m.tiles.get(m.selected_tile);
}

auto sprite::Editor::update(void) noexcept -> void {
    const backend::State& game_instance{ backend::State::instance() };
    sprite::State&        sprite_instance{ sprite::State::instance() };

    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_ESCAPE)) {
        sprite_instance.is_editing = !sprite_instance.is_editing;
    }

    if (!sprite_instance.is_editing) {
        return;
    }

    sync_level();

    swap_brushes();

    if (game_instance.mouse_state.any()) {
        draw_tiles();
    } else {
        show_brush();
    }
}

auto sprite::Editor::render(void) noexcept -> void {
    const sprite::State& instance{ sprite::State::instance() };

    if (instance.is_editing && !backend::State::instance().mouse_state.any()) {
        m.brush_tile.render(position, transform, instance.settings.get(combine("editor", "brush-transparency")));
    }
}

auto sprite::Editor::quit(void) noexcept -> void {
    m.brush_tile.destroy();
    m.tiles.destroy();
}

auto sprite::Editor::load_block_data(void) noexcept -> void {
    const toml::table block_groups{ toml::deserialize(BLOCK_GROUPS_PATH) };

    for (const auto& block_group : BLOCK_GROUPS_ORDER) {
        auto array{ block_groups.get_as<toml::array>(block_group) };

        const auto keys = std::ranges::subrange(array->begin(), array->end())
                          | std::views::transform([](auto&& item) { return std::string{ item.value_or(std::string{ "" }) }; })
                          | std::ranges::to<std::vector>();

        m.tile_groups.append_range(keys);
        m.tile_group_boundaries.emplace_back(m.tile_groups.size());
    }

    const toml::table ground_variants{ toml::deserialize(GROUND_VARIANTS_PATH) };

    m.variants = std::ranges::subrange(ground_variants.begin(), ground_variants.end()) | std::views::transform([](auto&& item) {
        return std::make_pair(std::string{ item.first.str() }, static_cast<usize>(item.second.as_integer()->get()));
    }) | std::ranges::to<std::unordered_map>();
}

auto sprite::Editor::sync_level(void) noexcept -> void {
    backend::State& game_instance{ backend::State::instance() };
    sprite::State&  sprite_instance{ sprite::State::instance() };

#ifndef __EMSCRIPTEN__
    static std::atomic<bool>     is_file_dialog_open{ false };
    static std::filesystem::path selected_level_path{};
    static enum class DialogType {
        None,
        Open,
        Save,
    } dialog_type{};

#if defined(__APPLE__)
    const bool control_held{ (game_instance.mods & SDL_KMOD_GUI) != 0 };
#else
    const bool control_held{ (game_instance.mods & SDL_KMOD_CTRL) != 0 };
#endif

    const SDL_DialogFileFilter filters[]{ { "Super Mario World Screen", "smw-screen" } };

    if (control_held && game_instance.keys.contains(SDL_SCANCODE_O) && !is_file_dialog_open.load()) {
        is_file_dialog_open.store(true);

        ::SDL_ShowOpenFileDialog([](void* userdata, const char* const* files, int count) {
            auto* open_flag = static_cast<std::atomic<b8>*>(userdata);

            if (files && files[0]) {
                selected_level_path = files[0];
                dialog_type         = DialogType::Open;
            }

            open_flag->store(false);
        }, &is_file_dialog_open, nullptr, filters, std::size(filters), nullptr, false);
    }

    if (control_held && game_instance.keys.contains(SDL_SCANCODE_S) && !is_file_dialog_open.load()) {
        is_file_dialog_open.store(true);

        game_instance.keys.erase(SDL_SCANCODE_S);  // mario will move due to this so cancel that

        ::SDL_ShowSaveFileDialog([](void* userdata, const char* const* files, int count) {
            auto* open_flag = static_cast<std::atomic<b8>*>(userdata);

            if (files && files[0]) {
                selected_level_path = files[0];
                dialog_type         = DialogType::Save;
            }

            open_flag->store(false);
        }, &is_file_dialog_open, nullptr, filters, std::size(filters), nullptr);
    }

    switch (dialog_type) {
    case DialogType::Open: {
        std::ifstream input_file{ selected_level_path, std::ios::binary | std::ios::ate };

        const std::streamsize compressed_size{ input_file.tellg() };
        input_file.seekg(0, std::ios::beg);

        std::vector<u8> compressed_data(compressed_size);
        input_file.read(reinterpret_cast<char*>(compressed_data.data()), compressed_size);

        const usize total_raw_size{ ::ZSTD_getFrameContentSize(compressed_data.data(), compressed_size) };
        telemetry::validate(total_raw_size != ZSTD_CONTENTSIZE_ERROR, "not a valid ZSTD file");

        std::vector<u8> decompressed_raw(total_raw_size);

        const usize decompress_result{ ::ZSTD_decompress(decompressed_raw.data(), total_raw_size, compressed_data.data(), compressed_size) };
        telemetry::validate(!::ZSTD_isError(decompress_result), "decompression failed");

        u8 pattern[]{ 0, 0 };

        const auto header_end{ std::search(decompressed_raw.begin(), decompressed_raw.end(), std::begin(pattern), std::end(pattern)) };
        telemetry::validate(header_end != decompressed_raw.end(), "Header separator not found in decompressed data");

        usize offset{ static_cast<usize>(std::distance(decompressed_raw.begin(), header_end) + 2) };

        sprite_instance.level.clear();

        while (offset < decompressed_raw.size()) {
            auto tile_end = std::find(decompressed_raw.begin() + offset, decompressed_raw.end(), '\0');

            if (tile_end != decompressed_raw.end()) {
                std::vector<uint8_t> tile_data(decompressed_raw.begin() + offset, tile_end);
                sprite_instance.level.push_back(std::string(tile_data.begin(), tile_data.end()));

                offset = std::distance(decompressed_raw.begin(), tile_end) + 1;
            } else {
                break;
            }
        }

        sprite_instance.tiles_need_to_reset = true;

        dialog_type = DialogType::None;
        break;
    };

    case DialogType::Save: {
        std::vector<u8> raw{};

        const std::string header{ std::format("MCWD {}x{}", sprite_instance.level_size.x, sprite_instance.level_size.y) };
        raw.insert(raw.end(), header.begin(), header.end());
        raw.push_back('\0');
        raw.push_back('\0');

        for (const auto& tile : sprite_instance.level) {
            raw.insert(raw.end(), tile.begin(), tile.end());
            raw.push_back('\0');
        }

        const usize          max_size{ ::ZSTD_compressBound(raw.size()) };
        std::vector<uint8_t> compressed(max_size);

        const size_t result{ ::ZSTD_compress(compressed.data(), max_size, raw.data(), raw.size(), 3) };
        telemetry::validate(!::ZSTD_isError(result), "compressing file failed");

        std::ofstream output_file{ selected_level_path, std::ios::binary };
        output_file.write(reinterpret_cast<const char*>(compressed.data()), result);

        dialog_type = DialogType::None;

        break;
    }

    default: break;
    };
#endif // !__EMSCRIPTEN__
}

auto sprite::Editor::show_brush(void) noexcept -> void {
    const backend::State& game_instance{ backend::State::instance() };
    const sprite::State&  sprite_instance{ sprite::State::instance() };

    const glm::vec2 mouse_world{
        game_instance.mouse_position.x - game_instance.renderer_size.x / 2 + sprite_instance.camera_position.x,
        game_instance.mouse_position.y - game_instance.renderer_size.y / 2 + sprite_instance.camera_position.y,
    };

    const f64 x_start{ -((sprite_instance.grid_size.x - 1) * sprite_instance.scaled_tile) / 2 };
    const f64 y_start{ -((sprite_instance.grid_size.y - 1) * sprite_instance.scaled_tile) / 2 };

    const f64 snapped_x = std::round((mouse_world.x - x_start) / sprite_instance.scaled_tile) * sprite_instance.scaled_tile + x_start;
    const f64 snapped_y = std::round((mouse_world.y - y_start) / sprite_instance.scaled_tile) * sprite_instance.scaled_tile + y_start;

    position = {
        snapped_x - sprite_instance.camera_position.x + game_instance.renderer_size.x / 2,
        snapped_y - sprite_instance.camera_position.y + game_instance.renderer_size.y / 2,
    };
}

auto sprite::Editor::swap_brushes(void) noexcept -> void {
    const backend::State& game_instance{ backend::State::instance() };
    const sprite::State&  sprite_instance{ sprite::State::instance() };

    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_1)) {
        update_brush(0);
    }
    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_2)) {
        update_brush(1);
    }
    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_3)) {
        update_brush(2);
    }
    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_4)) {
        update_brush(3);
    }
    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_5)) {
        update_brush(4);
    }
    if (game_instance.just_pressed_keys.contains(SDL_SCANCODE_6)) {
        update_brush(5);
    }

    if (game_instance.keys.contains(SDL_SCANCODE_E)) {
        const usize tile_index{ sprite::word_coordinate_to_tile_index(game_instance.mouse_position + sprite_instance.camera_position) };

        m.selected_tile = sprite_instance.level[tile_index];
        m.brush_tile    = m.tiles.get(m.selected_tile);
    }
}

auto sprite::Editor::update_brush(const usize group_index) noexcept -> void {
    const backend::State& instance{ backend::State::instance() };

    const usize lower_bound{ (group_index == 0) ? 0UL : m.tile_group_boundaries[group_index - 1] };
    const usize upper_bound{ m.tile_group_boundaries[group_index] };

    if (m.tile_group_brush < lower_bound || m.tile_group_brush >= upper_bound) {
        m.tile_group_brush = lower_bound;
    } else {
        const isize range{ static_cast<isize>(upper_bound - lower_bound) };

        isize relative_idx{ static_cast<isize>(m.tile_group_brush) - static_cast<isize>(lower_bound) };

        if (instance.mods & SDL_KMOD_SHIFT) {
            relative_idx--;
        } else {
            relative_idx++;
        }

        relative_idx = (relative_idx % range + range) % range;

        m.tile_group_brush = lower_bound + static_cast<usize>(relative_idx);
    }

    m.selected_tile = m.tile_groups[m.tile_group_brush];

    m.brush_tile = m.tiles.get(m.selected_tile + (m.variants.contains(m.selected_tile) ? "-0" : ""));
}

auto sprite::Editor::draw_tiles(void) noexcept -> void {
    const backend::State& game_instance{ backend::State::instance() };
    sprite::State&        sprite_instance{ sprite::State::instance() };

    const usize       tile_index{ sprite::word_coordinate_to_tile_index(game_instance.mouse_position + sprite_instance.camera_position) };
    const std::string tile_name{ sprite_instance.level[tile_index] };

    if (game_instance.mouse_state[0] && !sprite_instance.level[tile_index].contains(m.selected_tile)) {
        if (m.variants.contains(m.selected_tile)) {
            sprite_instance.level[tile_index] = std::format("{}-{}", m.selected_tile, pseudorandom::range(0UL, m.variants.at(m.selected_tile)));
        } else {
            sprite_instance.level[tile_index] = m.selected_tile;
        }

        sprite_instance.tiles_need_to_reset = true;
    } else if (game_instance.mouse_state[2]) {
        sprite_instance.level[tile_index] = "air";

        sprite_instance.tiles_need_to_reset = true;
    }
}
