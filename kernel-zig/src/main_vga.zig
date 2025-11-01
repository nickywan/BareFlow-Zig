const std = @import("std");

// VGA text buffer at 0xB8000
const VGA_BUFFER = @as(*volatile [25][80]u16, @ptrFromInt(0xB8000));

pub fn panic(_: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    while (true) {
        asm volatile ("hlt");
    }
}

export fn kernel_main() void {
    // Clear VGA screen
    var row: usize = 0;
    while (row < 25) : (row += 1) {
        var col: usize = 0;
        while (col < 80) : (col += 1) {
            VGA_BUFFER[row][col] = 0x0F20; // White on black, space
        }
    }

    // Write success message
    const msg = "BareFlow Zig 64-bit SUCCESS!";
    var i: usize = 0;
    while (i < msg.len) : (i += 1) {
        VGA_BUFFER[0][i] = 0x0F00 | @as(u16, msg[i]);
    }

    // Write second line
    const msg2 = "All boot problems solved!";
    i = 0;
    while (i < msg2.len) : (i += 1) {
        VGA_BUFFER[2][i] = 0x0A00 | @as(u16, msg2[i]); // Green on black
    }

    // Halt
    while (true) {
        asm volatile ("hlt");
    }
}