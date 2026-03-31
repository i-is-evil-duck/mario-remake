const std = @import("std");
const rl = @import("raylib");
const level = @import("level.zig");

pub const Player = struct {
    x: f32 = 100,
    y: f32 = 300,
    vx: f32 = 0,
    vy: f32 = 0,
    is_jumping: bool = false,
    facing_right: bool = true,
    is_small: bool = true,
    is_fire: bool = false,
    invulnerable: f32 = 0,
    grounded: bool = false,

    pub fn update(p: *Player, lvl: *level.Level) void {
        var move_left = false;
        var move_right = false;
        var jump = false;

        if (rl.isKeyDown(.a) or rl.isKeyDown(.left)) {
            move_left = true;
        }
        if (rl.isKeyDown(.d) or rl.isKeyDown(.right)) {
            move_right = true;
        }
        if (rl.isKeyDown(.space) or rl.isKeyDown(.w) or rl.isKeyDown(.up)) {
            jump = true;
        }

        if (move_left) {
            p.vx = -level.MOVE_SPEED;
            p.facing_right = false;
        } else if (move_right) {
            p.vx = level.MOVE_SPEED;
            p.facing_right = true;
        } else {
            p.vx = 0;
        }

        if (jump and !p.is_jumping) {
            p.vy = level.JUMP_VELOCITY;
            p.is_jumping = true;
        }

        p.vy += level.GRAVITY;
        if (p.vy > level.MAX_FALL_SPEED) {
            p.vy = level.MAX_FALL_SPEED;
        }

        const height: f32 = if (p.is_small) 16 else 32;
        const width: f32 = 16;

        const new_x = p.x + p.vx;

        if (new_x >= 0 and new_x < @as(f32, @floatFromInt(lvl.width * level.TILE_SIZE)) - width) {
            const tile_y_top = @as(usize, @intFromFloat(p.y + 1)) / @as(usize, @intCast(level.TILE_SIZE));
            const tile_y_bottom = @as(usize, @intFromFloat(p.y + height - 1)) / @as(usize, @intCast(level.TILE_SIZE));

            var can_move = true;

            if (p.vx > 0) {
                const tile_x_right = @as(usize, @intFromFloat(new_x + width)) / @as(usize, @intCast(level.TILE_SIZE));
                if (tile_x_right < lvl.width) {
                    const tile_top = lvl.getTile(tile_x_right, tile_y_top);
                    const tile_bottom = lvl.getTile(tile_x_right, tile_y_bottom);
                    if ((tile_top != level.TILE_AIR and tile_top != level.TILE_INVISIBLE and tile_top != level.TILE_QUESTION + 1 and tile_top != level.TILE_PLATFORM) or
                        (tile_bottom != level.TILE_AIR and tile_bottom != level.TILE_INVISIBLE and tile_bottom != level.TILE_QUESTION + 1 and tile_bottom != level.TILE_PLATFORM))
                    {
                        can_move = false;
                    }
                }
            } else if (p.vx < 0) {
                const tile_x_left = @as(usize, @intFromFloat(new_x)) / @as(usize, @intCast(level.TILE_SIZE));
                if (tile_x_left < lvl.width) {
                    const tile_top = lvl.getTile(tile_x_left, tile_y_top);
                    const tile_bottom = lvl.getTile(tile_x_left, tile_y_bottom);
                    if ((tile_top != level.TILE_AIR and tile_top != level.TILE_INVISIBLE and tile_top != level.TILE_QUESTION + 1 and tile_top != level.TILE_PLATFORM) or
                        (tile_bottom != level.TILE_AIR and tile_bottom != level.TILE_INVISIBLE and tile_bottom != level.TILE_QUESTION + 1 and tile_bottom != level.TILE_PLATFORM))
                    {
                        can_move = false;
                    }
                }
            }

            if (can_move) {
                p.x = new_x;
            }
        }

        const new_y = p.y + p.vy;

        if (new_y > @as(f32, @floatFromInt(lvl.height * level.TILE_SIZE)) - height) {
            p.y = @as(f32, @floatFromInt(lvl.height * level.TILE_SIZE)) - height;
            p.vy = 0;
            p.is_jumping = false;
            p.grounded = true;
        } else if (new_y < 0) {
            p.y = 0;
            p.vy = 0;
        } else {
            const tile_y_bottom = @as(usize, @intFromFloat(new_y + height)) / @as(usize, @intCast(level.TILE_SIZE));
            const tile_x = @as(usize, @intFromFloat(p.x + width / 2)) / @as(usize, @intCast(level.TILE_SIZE));

            if (tile_y_bottom < lvl.height and tile_x < lvl.width) {
                const tile_bottom = lvl.getTile(tile_x, tile_y_bottom);
                const is_platform = tile_bottom == level.TILE_PLATFORM;
                const is_solid = tile_bottom != level.TILE_AIR and tile_bottom != level.TILE_QUESTION and tile_bottom != level.TILE_QUESTION + 1 and tile_bottom != level.TILE_INVISIBLE;

                if (is_solid and (p.vy >= 0)) {
                    if (is_platform) {
                        if (p.y + height <= @as(f32, @floatFromInt(tile_y_bottom * level.TILE_SIZE))) {
                            p.y = new_y;
                            p.grounded = false;
                        } else {
                            p.y = @as(f32, @floatFromInt(tile_y_bottom * level.TILE_SIZE)) - height;
                            p.vy = 0;
                            p.is_jumping = false;
                            p.grounded = true;
                        }
                    } else {
                        p.y = @as(f32, @floatFromInt(tile_y_bottom * level.TILE_SIZE)) - height;
                        p.vy = 0;
                        p.is_jumping = false;
                        p.grounded = true;
                    }
                } else {
                    p.y = new_y;
                    p.grounded = false;
                }
            } else {
                p.y = new_y;
                p.grounded = false;
            }
        }

        if (p.invulnerable > 0) {
            p.invulnerable -= 1;
        }
    }

    pub fn draw(p: *Player, camera_x: f32) void {
        if (p.invulnerable > 0 and (@as(u32, @intFromFloat(p.invulnerable)) % 4) < 2) {
            return;
        }

        const height: c_int = if (p.is_small) 16 else 32;
        const px = @as(c_int, @intFromFloat(p.x - camera_x));
        const py = @as(c_int, @intFromFloat(p.y));

        const color: rl.Color = if (p.is_fire) rl.Color{ .r = 255, .g = 100, .b = 0, .a = 255 } else rl.Color{ .r = 255, .g = 0, .b = 0, .a = 255 };

        rl.drawRectangle(px, py, 16, height, color);

        const eye_x: c_int = if (p.facing_right) px + 10 else px + 2;
        rl.drawRectangle(eye_x, py + 2, 4, 4, rl.Color{ .r = 0, .g = 0, .b = 0, .a = 255 });

        if (!p.is_small) {
            const hat_color = if (p.is_fire) rl.Color{ .r = 255, .g = 255, .b = 0, .a = 255 } else rl.Color{ .r = 255, .g = 0, .b = 0, .a = 255 };
            rl.drawRectangle(px - 1, py - 4, 18, 6, hat_color);
        }
    }

    pub fn damage(p: *Player) bool {
        if (p.invulnerable > 0) return false;

        if (p.is_small) {
            p.invulnerable = 120;
            return true;
        } else {
            p.is_small = true;
            p.is_fire = false;
            p.invulnerable = 120;
            return false;
        }
    }

    pub fn grow(p: *Player) void {
        p.is_small = false;
        p.invulnerable = 60;
    }

    pub fn getHitbox(p: *Player) struct { x: f32, y: f32, w: f32, h: f32 } {
        const h: f32 = if (p.is_small) 16.0 else 32.0;
        return .{ .x = p.x, .y = p.y, .w = 16.0, .h = h };
    }
};
