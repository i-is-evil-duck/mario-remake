const std = @import("std");
const rl = @import("raylib");
const level = @import("level.zig");

pub const EnemyType = enum {
    goomba,
    koopa,
};

pub const Goomba = struct {
    x: f32,
    y: f32,
    vx: f32 = -1,
    vy: f32 = 0,
    alive: bool = true,
    dead: bool = false,
    death_timer: f32 = 0,
    spawn_delay: f32 = 0,

    pub fn update(g: *Goomba, lvl: *level.Level) void {
        if (g.spawn_delay > 0) {
            g.spawn_delay -= 1;
            return;
        }

        if (!g.alive) return;

        if (g.dead) {
            g.death_timer -= 1;
            return;
        }

        g.vy += level.GRAVITY;
        if (g.vy > level.MAX_FALL_SPEED) g.vy = level.MAX_FALL_SPEED;

        const new_x = g.x + g.vx;
        const new_y = g.y + g.vy;

        const tile_y_bottom = @as(usize, @intFromFloat(new_y + 16)) / @as(usize, @intCast(level.TILE_SIZE));
        const tile_y_mid = @as(usize, @intFromFloat(g.y + 8)) / @as(usize, @intCast(level.TILE_SIZE));

        if (tile_y_bottom < lvl.height) {
            const tile = lvl.getTile(@as(usize, @intFromFloat(new_x + 8)), tile_y_bottom);
            if (tile != level.TILE_AIR) {
                g.y = @as(f32, @floatFromInt(tile_y_bottom * level.TILE_SIZE)) - 16;
                g.vy = 0;
            } else {
                g.y = new_y;
            }
        }

        var hit_wall = false;
        if (new_x < 0) {
            g.vx = -g.vx;
            hit_wall = true;
        } else if (new_x > @as(f32, @floatFromInt(lvl.width * level.TILE_SIZE)) - 16) {
            g.vx = -g.vx;
            hit_wall = true;
        } else {
            const check_x: usize = if (g.vx > 0)
                @as(usize, @intFromFloat(new_x + 16))
            else
                @as(usize, @intFromFloat(new_x));

            const wall_tile_x = check_x / @as(usize, @intCast(level.TILE_SIZE));

            if (wall_tile_x < lvl.width and tile_y_mid < lvl.height) {
                const wall_tile = lvl.getTile(wall_tile_x, tile_y_mid);
                if (wall_tile != level.TILE_AIR) {
                    g.vx = -g.vx;
                    hit_wall = true;
                }
            }
        }

        if (!hit_wall) {
            g.x += g.vx;
        }
    }

    pub fn draw(g: *Goomba, camera_x: f32) void {
        if (!g.alive or g.spawn_delay > 0) return;

        const px = @as(c_int, @intFromFloat(g.x - camera_x));
        const py = @as(c_int, @intFromFloat(g.y));

        if (g.dead) {
            rl.drawRectangle(px, py + 8, 16, 8, rl.Color{ .r = 139, .g = 69, .b = 19, .a = 255 });
        } else {
            rl.drawRectangle(px, py, 16, 16, rl.Color{ .r = 139, .g = 69, .b = 19, .a = 255 });

            const eye_y = if (g.vy != 0) py + 2 else py + 4;
            rl.drawRectangle(px + 3, @as(c_int, @intCast(eye_y)), 4, 4, rl.Color{ .r = 0, .g = 0, .b = 0, .a = 255 });
            rl.drawRectangle(px + 9, @as(c_int, @intCast(eye_y)), 4, 4, rl.Color{ .r = 0, .g = 0, .b = 0, .a = 255 });
        }
    }

    pub fn getHitbox(g: Goomba) struct { x: f32, y: f32, w: f32, h: f32 } {
        if (g.dead) {
            return .{ .x = g.x, .y = g.y + 8, .w = 16, .h = 8 };
        }
        return .{ .x = g.x, .y = g.y, .w = 16, .h = 16 };
    }

    pub fn stomp(g: *Goomba) void {
        g.dead = true;
        g.death_timer = 20;
        g.vy = -3;
    }
};

pub const Enemies = struct {
    goombas: [64]Goomba = undefined,
    count: usize = 0,

    pub fn spawnGoomba(e: *Enemies, x: f32, y: f32) void {
        if (e.count < e.goombas.len) {
            e.goombas[e.count] = .{
                .x = x,
                .y = y,
                .spawn_delay = 0,
            };
            e.count += 1;
        }
    }

    pub fn update(e: *Enemies, lvl: *level.Level) void {
        for (0..e.count) |i| {
            e.goombas[i].update(lvl);

            if (e.goombas[i].dead and e.goombas[i].death_timer <= 0) {
                e.goombas[i].alive = false;
            }
        }
    }

    pub fn draw(e: *Enemies, camera_x: f32) void {
        for (0..e.count) |i| {
            e.goombas[i].draw(camera_x);
        }
    }

    pub fn reset(e: *Enemies) void {
        e.count = 0;
    }
};
