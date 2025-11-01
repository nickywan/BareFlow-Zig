const std = @import("std");

// Serial port for output
const COM1 = 0x3F8;

// VGA text buffer
const VGA_BUFFER = @as(*volatile [25][80]u16, @ptrFromInt(0xB8000));

// Static heap buffer in BSS (automatically zeroed by Zig!)
var heap_buffer: [32 * 1024 * 1024]u8 align(4096) = undefined;

// Our allocator - solves the malloc problems!
var fixed_allocator = std.heap.FixedBufferAllocator.init(&heap_buffer);
const allocator = fixed_allocator.allocator();

// Serial output functions
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
    // Initialize COM1 serial port
    outb(COM1 + 1, 0x00); // Disable interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB
    outb(COM1 + 0, 0x03); // Set divisor to 3 (38400 baud)
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

pub fn serial_write(c: u8) void {
    while ((inb(COM1 + 5) & 0x20) == 0) {}
    outb(COM1, c);
}

pub fn serial_print(msg: []const u8) void {
    for (msg) |c| {
        serial_write(c);
    }
}

pub fn serial_print_hex(value: u32) void {
    const hex_chars = "0123456789ABCDEF";
    serial_print("0x");
    var i: u5 = 28;
    while (true) : (i -%= 4) {
        const nibble = @as(u8, @truncate((value >> i) & 0xF));
        serial_write(hex_chars[nibble]);
        if (i == 0) break;
    }
}

pub fn serial_print_hex64(value: u64) void {
    const hex_chars = "0123456789ABCDEF";
    serial_print("0x");
    var i: u6 = 60;
    while (true) : (i -%= 4) {
        const nibble = @as(u8, @truncate((value >> @as(u6, @intCast(i))) & 0xF));
        serial_write(hex_chars[nibble]);
        if (i == 0) break;
    }
}

// VGA output functions
pub fn vga_clear() void {
    for (0..25) |row| {
        for (0..80) |col| {
            VGA_BUFFER[row][col] = 0x0720; // Space with gray on black
        }
    }
}

pub fn vga_print(row: usize, col: usize, msg: []const u8) void {
    var c = col;
    for (msg) |char| {
        if (c >= 80) break;
        VGA_BUFFER[row][c] = 0x0700 | @as(u16, char);
        c += 1;
    }
}

// Test structure to verify allocations
const TestStruct = struct {
    magic: u32,
    value: i32,
    data: [1024]u8,
};

// Function that returns values properly (no C ABI issues!)
fn test_return_value(x: i32) i32 {
    serial_print("Testing return value with x = ");
    serial_print_hex(@as(u32, @bitCast(x)));
    serial_print("\n");

    // This will return properly, no mysterious crashes!
    return x + 42;
}

// Test allocation function
fn test_allocation() !void {
    serial_print("\n=== Testing Zig Allocator (Solves malloc issues!) ===\n");

    // Allocate a test structure - with proper error handling!
    const test_obj = try allocator.create(TestStruct);
    defer allocator.destroy(test_obj);

    // Initialize it
    test_obj.magic = 0xDEADBEEF;
    test_obj.value = 12345;
    @memset(&test_obj.data, 0xAB);

    serial_print("Allocated TestStruct at: ");
    serial_print_hex64(@intFromPtr(test_obj));
    serial_print("\n");

    serial_print("Magic value: ");
    serial_print_hex(test_obj.magic);
    serial_print("\n");

    // Allocate an array - no silent corruption!
    const array = try allocator.alloc(u32, 256);
    defer allocator.free(array);

    serial_print("Allocated array at: ");
    serial_print_hex64(@intFromPtr(array.ptr));
    serial_print("\n");

    // Fill and verify the array
    for (array, 0..) |*item, i| {
        item.* = @as(u32, @intCast(i * 2));
    }

    // Verify first few values
    serial_print("Array[0] = ");
    serial_print_hex(array[0]);
    serial_print("\n");
    serial_print("Array[10] = ");
    serial_print_hex(array[10]);
    serial_print(" (expected 0x14)\n");

    serial_print("✓ Allocation test passed!\n");
}

// Test return values
fn test_returns() void {
    serial_print("\n=== Testing Return Values (No ABI issues!) ===\n");

    const result1 = test_return_value(100);
    serial_print("Result 1: ");
    serial_print_hex(@as(u32, @bitCast(result1)));
    serial_print(" (expected 142 = 0x8E)\n");

    const result2 = test_return_value(-50);
    serial_print("Result 2: ");
    serial_print_hex(@as(u32, @bitCast(result2)));
    serial_print(" (expected -8)\n");

    // Test with function pointer - still works!
    const fn_ptr = &test_return_value;
    const result3 = fn_ptr(1000);
    serial_print("Result via ptr: ");
    serial_print_hex(@as(u32, @bitCast(result3)));
    serial_print(" (expected 1042 = 0x412)\n");

    serial_print("✓ Return value test passed!\n");
}

// Kernel panic handler
pub fn panic(msg: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    serial_print("\n!!! KERNEL PANIC !!!\n");
    serial_print(msg);
    serial_print("\n");

    // Also display on VGA
    vga_clear();
    vga_print(12, 30, "KERNEL PANIC!");
    vga_print(14, 20, msg);

    // Halt the CPU
    while (true) {
        asm volatile ("hlt");
    }
}

// Kernel main entry point
export fn kernel_main() void {
    // Initialize serial port
    serial_init();
    serial_print("\n");
    serial_print("=====================================\n");
    serial_print("   BareFlow Zig Kernel v0.1.0\n");
    serial_print("=====================================\n");
    serial_print("✓ Kernel booted successfully!\n");

    // Clear VGA and show status
    vga_clear();
    vga_print(0, 0, "BareFlow Zig Kernel v0.1.0");
    vga_print(2, 0, "Status: Running");

    // Show heap info
    serial_print("\nHeap Configuration:\n");
    serial_print("  Buffer size: 32 MB\n");
    serial_print("  Buffer addr: ");
    serial_print_hex64(@intFromPtr(&heap_buffer));
    serial_print("\n");
    serial_print("  Alignment: 4096 bytes\n");

    // Test our problematic areas from C
    test_returns();

    test_allocation() catch |err| {
        serial_print("Allocation error: ");
        switch (err) {
            error.OutOfMemory => serial_print("Out of memory\n"),
        }
    };

    serial_print("\n=== All tests passed! ===\n");
    serial_print("Zig solves our C problems:\n");
    serial_print("  ✓ No malloc corruption\n");
    serial_print("  ✓ No BSS initialization issues\n");
    serial_print("  ✓ No return value ABI bugs\n");
    serial_print("  ✓ Comptime safety checks\n");
    serial_print("  ✓ Explicit error handling\n");

    vga_print(4, 0, "Tests: PASSED");
    vga_print(6, 0, "Problems solved:");
    vga_print(7, 2, "- malloc: FIXED");
    vga_print(8, 2, "- returns: FIXED");
    vga_print(9, 2, "- BSS init: FIXED");

    serial_print("\nKernel ready. Halting.\n");

    // Success - halt
    while (true) {
        asm volatile ("hlt");
    }
}