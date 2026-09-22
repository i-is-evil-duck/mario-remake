const std = @import("std"); pub fn main() void { for (std.builtin.CallingConvention.__enum, 0..) |e, i| { std.debug.print("{d}: {s}\n", .{i, @tagName(e)}); } }
