const std = @import("std");
const rl = @import("raylib");

pub const TILE_SIZE: c_int = 16;
pub const SCREEN_WIDTH: c_int = 512;
pub const SCREEN_HEIGHT: c_int = 448;

pub const GRAVITY: f32 = 0.5;
pub const JUMP_VELOCITY: f32 = -9.0;
pub const MOVE_SPEED: f32 = 3.0;
pub const MAX_FALL_SPEED: f32 = 8.0;

pub const TILE_AIR: u8 = 0;
pub const TILE_GROUND: u8 = 1;
pub const TILE_BRICK: u8 = 2;
pub const TILE_QUESTION: u8 = 3;
pub const TILE_PIPE: u8 = 4;
pub const TILE_PIPE_TOP: u8 = 5;
pub const TILE_INVISIBLE: u8 = 6;
pub const TILE_PLATFORM: u8 = 7;
pub const TILE_STAIR: u8 = 8;
pub const TILE_POLE: u8 = 9;
pub const TILE_FLAG: u8 = 10;

pub const PowerupType = enum {
    mushroom,
    fire_flower,
    star,
};

pub const Powerup = struct {
    x: f32,
    y: f32,
    vx: f32,
    vy: f32,
    kind: PowerupType,
    active: bool,
    spawn_delay: f32 = 0,
};

pub const Powerups = struct {
    items: [32]Powerup = undefined,
    count: usize = 0,

    pub fn spawn(p: *Powerups, x: f32, y: f32, kind: PowerupType) void {
        if (p.count < p.items.len) {
            p.items[p.count] = .{
                .x = x,
                .y = y,
                .vx = 0,
                .vy = 0,
                .kind = kind,
                .active = false,
                .spawn_delay = 30,
            };
            p.count += 1;
        }
    }

    pub fn update(p: *Powerups, lvl: *Level) void {
        for (0..p.count) |i| {
            var pw = &p.items[i];
            if (pw.spawn_delay > 0) {
                pw.spawn_delay -= 1;
                if (pw.spawn_delay == 0) {
                    pw.active = true;
                    pw.vy = -4;
                }
                continue;
            }

            if (!pw.active) continue;

            pw.vy += GRAVITY;
            if (pw.vy > MAX_FALL_SPEED) pw.vy = MAX_FALL_SPEED;

            const new_y = pw.y + pw.vy;
            const tile_y_bottom = @as(usize, @intFromFloat(new_y + 16)) / @as(usize, @intCast(TILE_SIZE));

            if (tile_y_bottom < lvl.height) {
                const tile_x = @as(usize, @intFromFloat(pw.x + 8)) / @as(usize, @intCast(TILE_SIZE));
                const tile = lvl.getTile(tile_x, tile_y_bottom);
                if (tile != TILE_AIR and tile != TILE_INVISIBLE and tile != TILE_QUESTION + 1 and tile != TILE_PLATFORM) {
                    pw.y = @as(f32, @floatFromInt(tile_y_bottom * TILE_SIZE)) - 16;
                    pw.vy = 0;
                } else {
                    pw.y = new_y;
                }
            }

            const new_x = pw.x + pw.vx;
            const tile_y = @as(usize, @intFromFloat(pw.y + 8)) / @as(usize, @intCast(TILE_SIZE));

            if (pw.vx > 0) {
                const tile_x_right = @as(usize, @intFromFloat(new_x + 16)) / @as(usize, @intCast(TILE_SIZE));
                if (tile_x_right < lvl.width) {
                    const tile = lvl.getTile(tile_x_right, tile_y);
                    if (tile != TILE_AIR and tile != TILE_INVISIBLE and tile != TILE_QUESTION + 1 and tile != TILE_PLATFORM) {
                        pw.vx = -pw.vx;
                    } else {
                        pw.x = new_x;
                    }
                } else {
                    pw.x = new_x;
                }
            } else if (pw.vx < 0) {
                const tile_x_left = @as(usize, @intFromFloat(new_x)) / @as(usize, @intCast(TILE_SIZE));
                if (tile_x_left < lvl.width) {
                    const tile = lvl.getTile(tile_x_left, tile_y);
                    if (tile != TILE_AIR and tile != TILE_INVISIBLE and tile != TILE_QUESTION + 1 and tile != TILE_PLATFORM) {
                        pw.vx = -pw.vx;
                    } else {
                        pw.x = new_x;
                    }
                } else {
                    pw.x = new_x;
                }
            }
        }
    }

    pub fn reset(p: *Powerups) void {
        p.count = 0;
    }
};

pub const Level = struct {
    width: usize = 32,
    height: usize = 28,
    tiles: []u8 = undefined,
    allocator: std.mem.Allocator,

    pub fn init(alloc: std.mem.Allocator) @This() {
        return .{
            .allocator = alloc,
            .tiles = alloc.alloc(u8, 32 * 28) catch undefined,
        };
    }

    pub fn deinit(self: *@This()) void {
        self.allocator.free(self.tiles);
    }

    pub fn resize(self: *@This(), new_width: usize, new_height: usize) void {
        self.allocator.free(self.tiles);
        self.width = new_width;
        self.height = new_height;
        self.tiles = self.allocator.alloc(u8, new_width * new_height) catch undefined;
    }

    pub fn getTile(self: *@This(), x: usize, y: usize) u8 {
        if (x >= self.width or y >= self.height) return TILE_AIR;
        return self.tiles[y * self.width + x];
    }

    pub fn setTile(self: *@This(), x: usize, y: usize, tile: u8) void {
        if (x < self.width and y < self.height) {
            self.tiles[y * self.width + x] = tile;
        }
    }

    pub fn hitBlock(self: *@This(), x: usize, y: usize) ?u8 {
        if (x >= self.width or y >= self.height) return null;
        const tile = self.tiles[y * self.width + x];
        if (tile == TILE_BRICK) {
            self.tiles[y * self.width + x] = TILE_AIR;
            return TILE_BRICK;
        }
        if (tile == TILE_QUESTION) {
            self.tiles[y * self.width + x] = TILE_QUESTION + 1;
            return TILE_QUESTION;
        }
        return null;
    }
};
