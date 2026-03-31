const std = @import("std");
const level = @import("level.zig");

pub const CHUNK_WIDTH_TILES: usize = 32;
pub const CHUNK_WIDTH: f32 = @as(f32, @floatFromInt(CHUNK_WIDTH_TILES * level.TILE_SIZE));

pub const Camera = struct {
    x: f32 = 0,
    target_x: f32 = 0,
    current_chunk: i32 = 0,

    pub fn update(c: *Camera, player_x: f32, level_width_tiles: usize) void {
        const level_width = @as(f32, @floatFromInt(level_width_tiles * level.TILE_SIZE));
        const screen_width: f32 = @floatFromInt(level.SCREEN_WIDTH);

        const right_edge = level_width - screen_width;

        c.target_x = player_x - screen_width / 2;

        if (c.target_x < 0) c.target_x = 0;
        if (c.target_x > right_edge) c.target_x = right_edge;

        const new_chunk = @as(i32, @intFromFloat(c.target_x / CHUNK_WIDTH));

        if (new_chunk != c.current_chunk and new_chunk >= 0 and @as(usize, @intCast(new_chunk)) * CHUNK_WIDTH_TILES < level_width_tiles) {
            c.current_chunk = new_chunk;
            c.target_x = @as(f32, @floatFromInt(new_chunk)) * CHUNK_WIDTH;
        }

        c.x = c.target_x;
    }

    pub fn getChunkStart(c: Camera) usize {
        return @as(usize, @intCast(c.current_chunk)) * CHUNK_WIDTH_TILES;
    }

    pub fn getChunkEnd(c: Camera, level_width_tiles: usize) usize {
        const end = (@as(usize, @intCast(c.current_chunk)) + 1) * CHUNK_WIDTH_TILES;
        if (end > level_width_tiles) return level_width_tiles;
        return end;
    }
};
