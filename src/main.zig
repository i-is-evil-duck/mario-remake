const std = @import("std");
const rl = @import("raylib");
const json = std.json;

const level = @import("level.zig");
const player = @import("player.zig");
const camera = @import("camera.zig");
const enemy = @import("enemy.zig");

const LEVEL_WIDTH_TILES: usize = 400;
const LEVEL_HEIGHT_TILES: usize = 28;
const LEVEL_WIDTH: usize = LEVEL_WIDTH_TILES * level.TILE_SIZE;
const LEVEL_HEIGHT: usize = LEVEL_HEIGHT_TILES * level.TILE_SIZE;

const GameState = enum {
    playing,
    game_over,
    win,
    builder,
};

var game_state: GameState = .playing;
var debug_enabled: bool = true;
var debug_show_collision: bool = false;

var builder_selected_tile: u8 = 1;
var builder_place_entity: ?[]const u8 = null;

var lvl: level.Level = undefined;
var cam: camera.Camera = undefined;
var p: player.Player = undefined;
var enemies: enemy.Enemies = undefined;
var powerups: level.Powerups = undefined;

fn rectsIntersect(x1: f32, y1: f32, w1: f32, h1: f32, x2: f32, y2: f32, w2: f32, h2: f32) bool {
    return x1 < x2 + w2 and x1 + w1 > x2 and y1 < y2 + h2 and y1 + h1 > y2;
}

fn loadLevelFromJson(alloc: std.mem.Allocator, path: []const u8) !void {
    const file = try std.fs.cwd().openFile(path, .{});
    defer file.close();

    const content = try file.readToEndAlloc(alloc, 1024 * 100);
    defer alloc.free(content);

    const parsed = try json.parseFromSlice(json.Value, alloc, content, .{});
    defer parsed.deinit();

    const root = parsed.value;

    var new_width: usize = lvl.width;
    var new_height: usize = lvl.height;

    if (root.object.get("width")) |w| {
        new_width = @as(usize, @intCast(w.integer));
    }
    if (root.object.get("height")) |h| {
        new_height = @as(usize, @intCast(h.integer));
    }

    if (new_width != lvl.width or new_height != lvl.height) {
        lvl.resize(new_width, new_height);
    }

    if (root.object.get("tiles")) |tiles_array| {
        for (tiles_array.array.items, 0..) |row, y| {
            for (row.array.items, 0..) |tile_val, x| {
                if (y < lvl.height and x < lvl.width) {
                    lvl.setTile(x, y, @as(u8, @intCast(tile_val.integer)));
                }
            }
        }
    }

    if (root.object.get("entities")) |entities| {
        for (entities.array.items) |ent| {
            const ent_type = ent.object.get("type").?.string;
            const ex = @as(f32, @floatFromInt(ent.object.get("x").?.integer));
            const ey = @as(f32, @floatFromInt(ent.object.get("y").?.integer));

            if (std.mem.eql(u8, ent_type, "goomba")) {
                enemies.spawnGoomba(ex, ey);
            } else if (std.mem.eql(u8, ent_type, "mushroom")) {
                powerups.spawn(ex, ey, .mushroom);
            } else if (std.mem.eql(u8, ent_type, "fire_flower")) {
                powerups.spawn(ex, ey, .fire_flower);
            }
        }
    }
}

fn saveLevelToJson(path: []const u8) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();

    var buf: [64]u8 = undefined;

    try file.writeAll("{\n");
    try file.writeAll("  \"width\": ");
    try file.writeAll(std.fmt.bufPrint(&buf, "{}", .{lvl.width}) catch unreachable);
    try file.writeAll(",\n");
    try file.writeAll("  \"height\": ");
    try file.writeAll(std.fmt.bufPrint(&buf, "{}", .{lvl.height}) catch unreachable);
    try file.writeAll(",\n");
    try file.writeAll("  \"tiles\": [\n");

    for (0..lvl.height) |y| {
        try file.writeAll("    [");
        for (0..lvl.width) |x| {
            try file.writeAll(std.fmt.bufPrint(&buf, "{}", .{lvl.getTile(x, y)}) catch unreachable);
            if (x < lvl.width - 1) try file.writeAll(", ");
        }
        try file.writeAll("]");
        if (y < lvl.height - 1) try file.writeAll(",");
        try file.writeAll("\n");
    }
    try file.writeAll("  ],\n");
    try file.writeAll("  \"entities\": [\n");

    for (0..enemies.count) |i| {
        const g = enemies.goombas[i];
        const gx = @as(i32, @intFromFloat(g.x));
        const gy = @as(i32, @intFromFloat(g.y));
        try file.writeAll(std.fmt.bufPrint(&buf, "    {{\"type\": \"goomba\", \"x\": {}, \"y\": {}}}", .{ gx, gy }) catch unreachable);
        if (i < enemies.count - 1 or powerups.count > 0) try file.writeAll(",");
        try file.writeAll("\n");
    }
    for (0..powerups.count) |i| {
        const pw = powerups.items[i];
        const px = @as(i32, @intFromFloat(pw.x));
        const py = @as(i32, @intFromFloat(pw.y));
        const typ = switch (pw.kind) {
            .mushroom => "mushroom",
            .fire_flower => "fire_flower",
            .star => "star",
        };
        try file.writeAll(std.fmt.bufPrint(&buf, "    {{\"type\": \"{s}\", \"x\": {}, \"y\": {}}}", .{ typ, px, py }) catch unreachable);
        if (i < powerups.count - 1) try file.writeAll(",");
        try file.writeAll("\n");
    }
    try file.writeAll("  ]\n");
    try file.writeAll("}\n");
}

fn initGame(alloc: std.mem.Allocator) void {
    lvl = level.Level.init(alloc);
    lvl.width = LEVEL_WIDTH_TILES;
    lvl.height = LEVEL_HEIGHT_TILES;

    cam = camera.Camera{};
    p = player.Player{};
    enemies = enemy.Enemies{};
    powerups = level.Powerups{};

    loadLevelFromJson(alloc, "level.json") catch |err| {
        std.debug.print("Failed to load level.json: {}, using default level\n", .{err});

        for (0..lvl.height) |y| {
            for (0..lvl.width) |x| {
                if (y == lvl.height - 1) {
                    lvl.setTile(x, y, level.TILE_GROUND);
                } else if (y == lvl.height - 3 and x > 3 and x < 12) {
                    lvl.setTile(x, y, level.TILE_BRICK);
                } else if (y == lvl.height - 4 and x > 3 and x < 12) {
                    lvl.setTile(x, y, level.TILE_QUESTION);
                } else if (y >= 10 and y <= 12 and x == 8) {
                    lvl.setTile(x, y, level.TILE_PIPE);
                } else if (y >= 10 and y <= 12 and x == 20) {
                    lvl.setTile(x, y, level.TILE_PIPE);
                } else if (y >= 10 and y <= 12 and x == 35) {
                    lvl.setTile(x, y, level.TILE_PIPE);
                } else if (y == lvl.height - 2 and x == 50) {
                    lvl.setTile(x, y, level.TILE_PIPE_TOP);
                } else if (y == lvl.height - 4 and x > 45 and x < 55) {
                    lvl.setTile(x, y, level.TILE_QUESTION);
                } else if (y == lvl.height - 2 and x > 70 and x < 90) {
                    lvl.setTile(x, y, level.TILE_GROUND);
                } else if (y == lvl.height - 3 and x > 70 and x < 90) {
                    const block_x: usize = @intCast(x);
                    if (block_x % 2 == 0) {
                        lvl.setTile(x, y, level.TILE_BRICK);
                    }
                }
            }
        }
    };
}

fn updateGame() void {
    if (game_state != .playing) return;

    p.update(&lvl);
    cam.update(p.x, lvl.width);
    enemies.update(&lvl);
    powerups.update(&lvl);

    const player_tile_x = @as(usize, @intFromFloat(p.x + 8)) / @as(usize, @intCast(level.TILE_SIZE));
    const player_tile_y = @as(usize, @intFromFloat(p.y + 8)) / @as(usize, @intCast(level.TILE_SIZE));
    if (player_tile_x < lvl.width and player_tile_y < lvl.height) {
        const tile = lvl.getTile(player_tile_x, player_tile_y);
        if (tile == level.TILE_FLAG or tile == level.TILE_POLE) {
            game_state = .win;
        }
    }

    const player_box = p.getHitbox();

    for (0..enemies.count) |i| {
        const g = enemies.goombas[i];
        if (!g.alive or g.spawn_delay > 0) continue;

        const enemy_box = g.getHitbox();

        if (rectsIntersect(player_box.x, player_box.y, player_box.w, player_box.h, enemy_box.x, enemy_box.y, enemy_box.w, enemy_box.h)) {
            if (p.vy > 0 and player_box.y + player_box.h - 8 < enemy_box.y + enemy_box.h / 2) {
                enemies.goombas[i].stomp();
                p.vy = -4;
            } else {
                if (p.damage()) {
                    game_state = .game_over;
                }
            }
        }
    }

    for (0..powerups.count) |i| {
        const pw = powerups.items[i];
        if (!pw.active) continue;

        const pw_box = .{ .x = pw.x, .y = pw.y, .w = 16.0, .h = 16.0 };

        if (rectsIntersect(player_box.x, player_box.y, player_box.w, player_box.h, pw_box.x, pw_box.y, pw_box.w, pw_box.h)) {
            powerups.items[i].active = false;

            switch (pw.kind) {
                .mushroom => {
                    if (p.is_small) {
                        p.grow();
                    }
                },
                .fire_flower => {
                    p.is_small = false;
                    p.is_fire = true;
                },
                .star => {
                    p.invulnerable = 300;
                },
            }
        }
    }

    if (p.y > @as(f32, @floatFromInt(LEVEL_HEIGHT))) {
        game_state = .game_over;
    }

    if (rl.isKeyPressed(.f1)) {
        debug_enabled = !debug_enabled;
    }
    if (rl.isKeyPressed(.f2)) {
        debug_show_collision = !debug_show_collision;
    }
    if (rl.isKeyPressed(.r)) {
        const alloc = std.heap.page_allocator;
        initGame(alloc);
        game_state = .playing;
    }
    if (rl.isKeyPressed(.b)) {
        if (game_state == .builder) {
            game_state = .playing;
        } else {
            game_state = .builder;
        }
    }
}

fn updateBuilder() void {
    const mouse_x = @as(f32, @floatFromInt(rl.getMouseX()));
    const mouse_y = @as(f32, @floatFromInt(rl.getMouseY()));

    const tile_x = @as(usize, @intFromFloat(mouse_x + cam.x)) / @as(usize, @intCast(level.TILE_SIZE));
    const tile_y = @as(usize, @intFromFloat(mouse_y)) / @as(usize, @intCast(level.TILE_SIZE));

    if (rl.isMouseButtonDown(.left)) {
        if (tile_x < lvl.width and tile_y < lvl.height) {
            lvl.setTile(tile_x, tile_y, builder_selected_tile);
        }
    }
    if (rl.isMouseButtonDown(.right)) {
        if (tile_x < lvl.width and tile_y < lvl.height) {
            lvl.setTile(tile_x, tile_y, level.TILE_AIR);
        }
    }

    if (rl.isKeyPressed(.one)) builder_selected_tile = 1;
    if (rl.isKeyPressed(.two)) builder_selected_tile = 2;
    if (rl.isKeyPressed(.three)) builder_selected_tile = 3;
    if (rl.isKeyPressed(.four)) builder_selected_tile = 4;
    if (rl.isKeyPressed(.five)) builder_selected_tile = 5;
    if (rl.isKeyPressed(.six)) builder_selected_tile = 7;
    if (rl.isKeyPressed(.seven)) builder_selected_tile = 8;
    if (rl.isKeyPressed(.eight)) builder_selected_tile = 9;
    if (rl.isKeyPressed(.nine)) builder_selected_tile = 10;

    if (rl.isKeyPressed(.g)) {
        if (tile_x < lvl.width and tile_y < lvl.height) {
            enemies.spawnGoomba(@as(f32, @floatFromInt(tile_x * 16)), @as(f32, @floatFromInt(tile_y * 16)));
        }
    }
    if (rl.isKeyPressed(.m)) {
        if (tile_x < lvl.width and tile_y < lvl.height) {
            powerups.spawn(@as(f32, @floatFromInt(tile_x * 16)), @as(f32, @floatFromInt(tile_y * 16)), .mushroom);
        }
    }
    if (rl.isKeyPressed(.f)) {
        if (tile_x < lvl.width and tile_y < lvl.height) {
            powerups.spawn(@as(f32, @floatFromInt(tile_x * 16)), @as(f32, @floatFromInt(tile_y * 16)), .fire_flower);
        }
    }

    if (rl.isKeyPressed(.s)) {
        saveLevelToJson("level.json") catch |err| {
            std.debug.print("Failed to save: {}\n", .{err});
        };
    }

    if (rl.isKeyPressed(.p)) {
        cam.x += 32 * 16;
    }
    if (rl.isKeyPressed(.o)) {
        cam.x -= 32 * 16;
        if (cam.x < 0) cam.x = 0;
    }
}

fn drawGame() void {
    rl.clearBackground(rl.Color{ .r = 107, .g = 140, .b = 255, .a = 255 });

    const start_x = cam.getChunkStart();
    var end_x = cam.getChunkEnd(lvl.width);

    const screen_width_tiles = 32;
    const camera_tile_x = @divFloor(@as(usize, @intFromFloat(cam.x)), 16);

    if (camera_tile_x + screen_width_tiles >= end_x and end_x < lvl.width) {
        end_x = end_x + camera.CHUNK_WIDTH_TILES;
        if (end_x > lvl.width) end_x = lvl.width;
    }

    for (start_x..end_x) |x| {
        for (0..lvl.height) |y| {
            const tile = lvl.getTile(x, y);
            if (tile != level.TILE_AIR and tile != level.TILE_INVISIBLE) {
                const px = @as(c_int, @intCast(x * level.TILE_SIZE)) - @as(c_int, @intFromFloat(cam.x));
                const py = @as(c_int, @intCast(y * level.TILE_SIZE));

                const color: rl.Color = if (tile == level.TILE_QUESTION + 1)
                    rl.Color{ .r = 255, .g = 215, .b = 0, .a = 100 }
                else switch (tile) {
                    level.TILE_GROUND => rl.Color{ .r = 139, .g = 69, .b = 19, .a = 255 },
                    level.TILE_BRICK => rl.Color{ .r = 205, .g = 92, .b = 92, .a = 255 },
                    level.TILE_QUESTION => rl.Color{ .r = 255, .g = 215, .b = 0, .a = 255 },
                    level.TILE_PIPE, level.TILE_PIPE_TOP => rl.Color{ .r = 34, .g = 139, .b = 34, .a = 255 },
                    level.TILE_PLATFORM => rl.Color{ .r = 186, .g = 133, .b = 84, .a = 255 },
                    level.TILE_STAIR => rl.Color{ .r = 139, .g = 119, .b = 101, .a = 255 },
                    level.TILE_POLE => rl.Color{ .r = 64, .g = 64, .b = 64, .a = 255 },
                    level.TILE_FLAG => rl.Color{ .r = 0, .g = 255, .b = 0, .a = 255 },
                    else => rl.Color{ .r = 128, .g = 128, .b = 128, .a = 255 },
                };

                rl.drawRectangle(px, py, level.TILE_SIZE, level.TILE_SIZE, color);

                if (debug_show_collision) {
                    rl.drawRectangleLines(px, py, level.TILE_SIZE, level.TILE_SIZE, rl.Color{ .r = 255, .g = 0, .b = 0, .a = 128 });
                }
            }
        }
    }

    for (0..powerups.count) |i| {
        const pw = powerups.items[i];
        if (!pw.active or pw.spawn_delay > 0) continue;

        const px = @as(c_int, @intFromFloat(pw.x - cam.x));
        const py = @as(c_int, @intFromFloat(pw.y));

        const color: rl.Color = switch (pw.kind) {
            .mushroom => rl.Color{ .r = 255, .g = 0, .b = 0, .a = 255 },
            .fire_flower => rl.Color{ .r = 255, .g = 128, .b = 0, .a = 255 },
            .star => rl.Color{ .r = 255, .g = 255, .b = 0, .a = 255 },
        };

        rl.drawRectangle(px, py, 16, 16, color);
    }

    enemies.draw(cam.x);
    if (game_state != .builder) {
        p.draw(cam.x);
    }

    if (debug_enabled) {
        rl.drawText("MARIO MAKER 2 - ZIG", 10, 10, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
        rl.drawText(rl.textFormat("Chunk: {}", .{cam.current_chunk}), 10, 25, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
        rl.drawText(rl.textFormat("Goombas: {}", .{enemies.count}), 10, 40, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });

        const state_str: [:0]const u8 = switch (game_state) {
            .playing => "PLAYING",
            .game_over => "GAME OVER",
            .win => "WIN!",
            .builder => "BUILDER",
        };
        rl.drawText(state_str, 10, 380, 20, switch (game_state) {
            .playing => rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 },
            .game_over => rl.Color{ .r = 255, .g = 0, .b = 0, .a = 255 },
            .win => rl.Color{ .r = 0, .g = 255, .b = 0, .a = 255 },
            .builder => rl.Color{ .r = 255, .g = 255, .b = 0, .a = 255 },
        });

        std.debug.print("Mario: ({d:.0}, {d:.0}) Chunk: {} Goombas: {}\n", .{ p.x, p.y, cam.current_chunk, enemies.count });
    }
}

fn drawBuilder() void {
    var y: c_int = 10;
    rl.drawText("BUILDER MODE", 10, y, 20, rl.Color{ .r = 255, .g = 255, .b = 0, .a = 255 });
    y += 25;

    const sel_idx = @as(c_int, @intCast(builder_selected_tile)) - 1;
    const tile_names = [_][:0]const u8{
        "Ground", "Brick", "Question", "Pipe", "PipeTop",
        "Invis",  "Plat",  "Stair",    "Pole", "Flag",
    };
    const name = tile_names[@intCast(sel_idx)];
    rl.drawText("Selected: ", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
    rl.drawText(name, 80, y, 10, rl.Color{ .r = 255, .g = 255, .b = 0, .a = 255 });
    y += 15;

    rl.drawText("LMB: Place tile  RMB: Erase", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
    y += 15;
    rl.drawText("1-9: Select tile", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
    y += 15;
    rl.drawText("G: Place Goomba  M: Mushroom  F: Fire Flower", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
    y += 15;
    rl.drawText("S: Save level  P/O: Scroll chunk", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
    y += 15;
    rl.drawText("B: Exit builder", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });
    y += 15;
    rl.drawText("R: Reset game", 10, y, 10, rl.Color{ .r = 255, .g = 255, .b = 255, .a = 255 });

    const mouse_x_float = @as(f32, @floatFromInt(rl.getMouseX()));
    const mouse_y_float = @as(f32, @floatFromInt(rl.getMouseY()));
    const hover_tile_x = @as(usize, @intFromFloat(mouse_x_float + cam.x)) / @as(usize, @intCast(level.TILE_SIZE));
    const hover_tile_y = @as(usize, @intFromFloat(mouse_y_float)) / @as(usize, @intCast(level.TILE_SIZE));

    const px = @as(c_int, @intCast(hover_tile_x * @as(usize, @intCast(level.TILE_SIZE)))) - @as(c_int, @intFromFloat(cam.x));
    const py = @as(c_int, @intCast(hover_tile_y * @as(usize, @intCast(level.TILE_SIZE))));

    rl.drawRectangleLines(px, py, level.TILE_SIZE, level.TILE_SIZE, rl.Color{ .r = 255, .g = 255, .b = 0, .a = 255 });
}

pub fn main() !void {
    const alloc = std.heap.page_allocator;

    rl.initWindow(level.SCREEN_WIDTH, level.SCREEN_HEIGHT, "Mario Maker 2 - Zig");
    defer rl.closeWindow();

    rl.setTargetFPS(60);

    initGame(alloc);

    std.debug.print("Mario Maker 2 - Zig\n", .{});
    std.debug.print("Controls: A/D or Arrows to move, Space/W/Up to jump\n", .{});
    std.debug.print("F1: Toggle debug, F2: Show collision, R: Reset\n", .{});
    std.debug.print("B: Builder mode\n", .{});
    std.debug.print("Level: {}x{} tiles ({}x{} pixels)\n\n", .{ LEVEL_WIDTH_TILES, LEVEL_HEIGHT_TILES, LEVEL_WIDTH, LEVEL_HEIGHT });

    while (!rl.windowShouldClose()) {
        if (game_state == .builder) {
            updateBuilder();
        } else {
            updateGame();
        }

        rl.beginDrawing();
        defer rl.endDrawing();

        drawGame();

        if (game_state == .builder) {
            drawBuilder();
        }
    }

    lvl.deinit();
}
