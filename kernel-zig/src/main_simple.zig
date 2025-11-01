const std = @import("std");

// Serial port for output
const COM1 = 0x3F8;

pub fn outb(port: u16, value: u8) void {
    asm volatile ("outb %[value], %[port]"
        :
        : [port] "{dx}" (port),
          [value] "{al}" (value),
    );
}

pub fn inb(port: u16) u8 {
    return asm volatile ("inb %[port], %[result]"
        : [result] "={al}" (-> u8),
        : [port] "{dx}" (port),
    );
}

pub fn serial_init() void {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

pub fn serial_write(c: u8) void {
    // Wait for THRE (bit 5) - Transmitter Holding Register Empty
    // Standard serial polling - only check THRE, not TEMT
    while ((inb(COM1 + 5) & 0x20) == 0) {}
    outb(COM1, c);
}

pub fn serial_print(msg: []const u8) void {
    for (msg) |c| {
        serial_write(c);
    }
}

pub fn panic(msg: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    serial_print("\nPANIC: ");
    serial_print(msg);
    serial_print("\n");
    while (true) {
        asm volatile ("hlt");
    }
}

export fn kernel_main() void {
    serial_init();

    // Test serial_print() with multiple strings
    serial_print("BareFlow Zig Kernel\n");
    serial_print("Serial I/O Test\n");
    serial_print("Boot complete!\n");

    while (true) {
        asm volatile ("hlt");
    }
}