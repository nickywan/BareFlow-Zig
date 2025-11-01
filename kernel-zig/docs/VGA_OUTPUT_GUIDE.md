# VGA Output Guide - QEMU and BareFlow Kernel

**Session**: 47 (Continuation - Option 2)
**Purpose**: Complete documentation of VGA text mode output in QEMU for BareFlow kernel
**Status**: ✅ IMPLEMENTED AND WORKING

---

## Table of Contents

1. [VGA Text Mode Overview](#vga-text-mode-overview)
2. [Memory Layout](#memory-layout)
3. [Character Format](#character-format)
4. [Color Codes](#color-codes)
5. [Implementation in Zig](#implementation-in-zig)
6. [Testing in QEMU](#testing-in-qemu)
7. [Troubleshooting](#troubleshooting)
8. [Advanced Usage](#advanced-usage)

---

## VGA Text Mode Overview

### What is VGA Text Mode?

VGA (Video Graphics Array) text mode is a **legacy display mode** that provides 80×25 character output in color. Each character on screen is stored as a **16-bit value** in a memory-mapped buffer.

### Key Specifications

- **Resolution**: 80 columns × 25 rows = 2000 characters
- **Buffer Size**: 2000 × 2 bytes = 4000 bytes (0xFA0)
- **Memory Address**: 0xB8000 (physical, identity-mapped)
- **Character Format**: 16-bit (attribute byte + ASCII byte)
- **Color Support**: 16 foreground colors + 8 background colors

---

## Memory Layout

### VGA Buffer Address

```
Physical Address: 0xB8000
Virtual Address:  0xB8000 (identity-mapped)
                  OR 0xFFFFFFFF800B8000 (higher-half kernel)
```

### Buffer Structure

```
Offset    Row  Col  Description
0x0000    0    0    First character (top-left)
0x0002    0    1    Second character
...
0x009E    0    79   Last character of first row
0x00A0    1    0    First character of second row
...
0x0F9E    24   79   Last character (bottom-right)
```

**Formula**:
```
Offset = (row × 80 + col) × 2
Address = 0xB8000 + Offset
```

---

## Character Format

### 16-bit Character Entry

Each character is stored as a **16-bit value**:

```
Bits 15-12: Background color (4 bits)
Bit  11:    Blink enable (1 bit) - usually 0
Bits 10-8:  Foreground color (3 bits)
Bit  7:     Foreground intensity (1 bit)
Bits 6-0:   ASCII character (7 bits) + extended bit

Simplified (most common):
Bits 15-8:  Attribute byte (color)
Bits 7-0:   Character byte (ASCII)
```

### Common Attribute Values

```
0x07: Light gray on black (standard)
0x0F: White on black (bright)
0x70: Black on light gray (inverted)
0x4F: White on red (error/warning)
0x2F: White on green (success)
```

### Example

```zig
// Character 'A' (0x41) with white on black (0x0F)
const vga_char: u16 = 0x0F41;

// Or built from parts:
const attribute: u8 = 0x0F;  // White on black
const character: u8 = 'A';   // ASCII 'A' = 0x41
const vga_char: u16 = (@as(u16, attribute) << 8) | character;
```

---

## Color Codes

### Foreground Colors (0-15)

| Code | Color           | Code | Color           |
|------|-----------------|------|-----------------|
| 0x0  | Black           | 0x8  | Dark Gray       |
| 0x1  | Blue            | 0x9  | Light Blue      |
| 0x2  | Green           | 0xA  | Light Green     |
| 0x3  | Cyan            | 0xB  | Light Cyan      |
| 0x4  | Red             | 0xC  | Light Red       |
| 0x5  | Magenta         | 0xD  | Light Magenta   |
| 0x6  | Brown           | 0xE  | Yellow          |
| 0x7  | Light Gray      | 0xF  | White           |

### Background Colors (0-7)

| Code | Color      |
|------|------------|
| 0x0  | Black      |
| 0x1  | Blue       |
| 0x2  | Green      |
| 0x3  | Cyan       |
| 0x4  | Red        |
| 0x5  | Magenta    |
| 0x6  | Brown      |
| 0x7  | Light Gray |

### Building Attribute Byte

```zig
// Formula: (background << 4) | foreground
const WHITE_ON_BLACK: u8 = (0x0 << 4) | 0xF;  // 0x0F
const BLACK_ON_RED: u8 = (0x4 << 4) | 0x0;    // 0x40
const YELLOW_ON_BLUE: u8 = (0x1 << 4) | 0xE;  // 0x1E
```

---

## Implementation in Zig

### Current Implementation (src/main.zig)

#### VGA Buffer Declaration

```zig
// VGA text buffer - volatile to prevent optimization
const VGA_BUFFER = @as(*volatile [25][80]u16, @ptrFromInt(0xB8000));
```

**Why volatile?**
- Prevents compiler from optimizing away "unused" writes
- Memory-mapped I/O requires every write to reach hardware
- Without volatile, Zig might cache writes and not update VGA

#### VGA Clear Function

```zig
pub fn vga_clear() void {
    for (0..25) |row| {
        for (0..80) |col| {
            VGA_BUFFER[row][col] = 0x0720; // Space with gray on black
        }
    }
}
```

**Breakdown**:
- `0x07`: Light gray on black (attribute)
- `0x20`: Space character (ASCII 32)
- Result: Clears screen to blank gray-on-black

#### VGA Print Function

```zig
pub fn vga_print(row: usize, col: usize, msg: []const u8) void {
    var c = col;
    for (msg) |char| {
        if (c >= 80) break;  // Stop at end of line
        VGA_BUFFER[row][c] = 0x0700 | @as(u16, char);
        c += 1;
    }
}
```

**Breakdown**:
- `0x0700`: Gray on black (attribute in upper byte)
- `char`: ASCII character (lower byte)
- `|` operator: Combines attribute and character
- **No word wrap**: Stops at column 79

### Usage Examples

```zig
// Clear screen
vga_clear();

// Print at top-left
vga_print(0, 0, "BareFlow Zig Kernel v0.1.0");

// Print at row 2, column 0
vga_print(2, 0, "Status: Running");

// Print centered (assuming 30-char message)
const center_col = (80 - 30) / 2;
vga_print(12, center_col, "KERNEL PANIC!");

// Print indented
vga_print(7, 2, "  - malloc: FIXED");
```

---

## Testing in QEMU

### Basic QEMU Command

```bash
# Boot with VGA output (default window)
qemu-system-x86_64 -M q35 -m 128 -cdrom bareflow.iso

# Boot with VNC server (headless)
qemu-system-x86_64 -M q35 -m 128 -cdrom bareflow.iso -vnc :0

# Boot with both serial and VGA
qemu-system-x86_64 -M q35 -m 128 -cdrom bareflow.iso \
    -serial file:serial.log
```

### Viewing VGA Output

#### Method 1: QEMU SDL Window (Default)

```bash
# Opens graphical window automatically
qemu-system-x86_64 -cdrom bareflow.iso
```

**Pros**: Immediate visual feedback
**Cons**: Requires X11/Wayland display server

#### Method 2: QEMU GTK Window

```bash
# Uses GTK+ display
qemu-system-x86_64 -display gtk -cdrom bareflow.iso
```

**Pros**: Better integration with desktop
**Cons**: Requires GTK libraries

#### Method 3: VNC Server (Headless)

```bash
# Start QEMU with VNC on port 5900
qemu-system-x86_64 -vnc :0 -cdrom bareflow.iso

# Connect with VNC viewer (separate terminal)
vncviewer localhost:5900
```

**Pros**: Works over network, no X11 needed on server
**Cons**: Requires VNC client

#### Method 4: Curses (Text Mode)

```bash
# Text-mode display in terminal
qemu-system-x86_64 -display curses -cdrom bareflow.iso
```

**Pros**: No graphical display needed
**Cons**: Limited color support, press Esc+2 to exit

### Capturing VGA Output

#### Screenshot with QEMU Monitor

```bash
# Start QEMU with monitor
qemu-system-x86_64 -monitor stdio -cdrom bareflow.iso

# In QEMU monitor console:
(qemu) screendump vga-output.ppm

# Convert to PNG (requires ImageMagick)
convert vga-output.ppm vga-output.png
```

#### Recording Video

```bash
# Record VGA output to video file
qemu-system-x86_64 -cdrom bareflow.iso -vnc :0

# Use vnc2video or similar tool to record VNC stream
vnc2video -o vga-recording.mp4 localhost:5900
```

---

## Troubleshooting

### Issue: VGA Output Not Visible

**Symptoms**: Blank screen, no text appears

**Possible Causes**:

1. **VGA buffer not mapped**
   ```zig
   // ❌ WRONG - No paging setup
   const VGA_BUFFER = @ptrFromInt(0xB8000);

   // ✅ CORRECT - Identity-mapped or higher-half
   const VGA_BUFFER = @as(*volatile [25][80]u16, @ptrFromInt(0xB8000));
   ```

2. **Missing volatile qualifier**
   ```zig
   // ❌ WRONG - Compiler may optimize away writes
   const VGA_BUFFER = @as(*[25][80]u16, @ptrFromInt(0xB8000));

   // ✅ CORRECT - Forces actual writes
   const VGA_BUFFER = @as(*volatile [25][80]u16, @ptrFromInt(0xB8000));
   ```

3. **Wrong attribute byte (black on black)**
   ```zig
   // ❌ WRONG - Invisible text!
   VGA_BUFFER[0][0] = 0x0041;  // Black 'A' on black background

   // ✅ CORRECT - Visible text
   VGA_BUFFER[0][0] = 0x0F41;  // White 'A' on black background
   ```

4. **QEMU not showing VGA**
   ```bash
   # ❌ WRONG - VNC :99 may not be visible
   qemu-system-x86_64 -vnc :99 -cdrom bareflow.iso

   # ✅ CORRECT - Default display
   qemu-system-x86_64 -cdrom bareflow.iso
   ```

### Issue: Garbage Characters

**Symptoms**: Random characters, corrupted output

**Possible Causes**:

1. **Wrong endianness**
   ```zig
   // ❌ WRONG - Reversed bytes
   VGA_BUFFER[0][0] = 0x4107;  // 0x41 (attr) 0x07 (char)

   // ✅ CORRECT - Attribute high, character low
   VGA_BUFFER[0][0] = 0x0741;  // 0x07 (attr) 0x41 (char)
   ```

2. **BSS not zeroed**
   - VGA buffer in .bss may contain garbage
   - Solution: Clear VGA at boot with `vga_clear()`

3. **Memory corruption**
   - Stack overflow writing to VGA buffer
   - Solution: Check stack size, use `-mno-red-zone`

### Issue: Colors Wrong

**Symptoms**: Unexpected colors, inverted display

**Possible Causes**:

1. **Swapped foreground/background**
   ```zig
   // Foreground in bits 0-3, background in bits 4-7
   const attr = (fg << 4) | bg;  // ❌ WRONG
   const attr = (bg << 4) | fg;  // ✅ CORRECT
   ```

2. **Blink bit set**
   ```zig
   // Bit 7 of attribute enables blinking (may appear as different color)
   const attr = 0x8F;  // ❌ Blinking white on black
   const attr = 0x0F;  // ✅ Static white on black
   ```

---

## Advanced Usage

### Custom Color Function

```zig
pub fn vga_print_colored(
    row: usize,
    col: usize,
    msg: []const u8,
    fg: u8,
    bg: u8
) void {
    var c = col;
    const attr = (bg << 4) | fg;
    for (msg) |char| {
        if (c >= 80) break;
        VGA_BUFFER[row][c] = (@as(u16, attr) << 8) | char;
        c += 1;
    }
}

// Usage
vga_print_colored(10, 20, "ERROR", 0xF, 0x4);  // White on red
vga_print_colored(11, 20, "SUCCESS", 0xF, 0x2);  // White on green
```

### Scrolling (Simple Implementation)

```zig
pub fn vga_scroll() void {
    // Move all rows up by 1
    for (1..25) |row| {
        for (0..80) |col| {
            VGA_BUFFER[row - 1][col] = VGA_BUFFER[row][col];
        }
    }

    // Clear bottom row
    for (0..80) |col| {
        VGA_BUFFER[24][col] = 0x0720;  // Space
    }
}
```

### Cursor Control (Port-based)

```zig
// Note: VGA cursor is separate from buffer writes
pub fn vga_set_cursor(row: usize, col: usize) void {
    const pos = row * 80 + col;

    // Cursor low byte (port 0x3D4 + 0x3D5)
    outb(0x3D4, 0x0F);
    outb(0x3D5, @as(u8, @truncate(pos & 0xFF)));

    // Cursor high byte
    outb(0x3D4, 0x0E);
    outb(0x3D5, @as(u8, @truncate((pos >> 8) & 0xFF)));
}
```

### Box Drawing

```zig
pub fn vga_draw_box(
    top: usize,
    left: usize,
    bottom: usize,
    right: usize
) void {
    // Box drawing characters (Code Page 437)
    const TL: u8 = 0xDA;  // ┌
    const TR: u8 = 0xBF;  // ┐
    const BL: u8 = 0xC0;  // └
    const BR: u8 = 0xD9;  // ┘
    const H: u8 = 0xC4;   // ─
    const V: u8 = 0xB3;   // │

    const attr: u16 = 0x0700;

    // Corners
    VGA_BUFFER[top][left] = attr | TL;
    VGA_BUFFER[top][right] = attr | TR;
    VGA_BUFFER[bottom][left] = attr | BL;
    VGA_BUFFER[bottom][right] = attr | BR;

    // Top and bottom edges
    for (left + 1..right) |col| {
        VGA_BUFFER[top][col] = attr | H;
        VGA_BUFFER[bottom][col] = attr | H;
    }

    // Left and right edges
    for (top + 1..bottom) |row| {
        VGA_BUFFER[row][left] = attr | V;
        VGA_BUFFER[row][right] = attr | V;
    }
}
```

---

## Current Kernel Usage

### Boot Sequence

```zig
// kernel_main() in src/main.zig

// 1. Clear VGA
vga_clear();

// 2. Show boot banner
vga_print(0, 0, "BareFlow Zig Kernel v0.1.0");
vga_print(2, 0, "Status: Running");

// 3. After tests complete
vga_print(4, 0, "Tests: PASSED");
vga_print(6, 0, "Problems solved:");
vga_print(7, 2, "- malloc: FIXED");
vga_print(8, 2, "- returns: FIXED");
vga_print(9, 2, "- BSS init: FIXED");
```

### Panic Handler

```zig
pub fn panic(msg: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    serial_print("\n!!! KERNEL PANIC !!!\n");
    serial_print(msg);
    serial_print("\n");

    // Display on VGA (centered)
    vga_clear();
    vga_print(12, 30, "KERNEL PANIC!");
    vga_print(14, 20, msg);

    // Halt CPU
    while (true) {
        asm volatile ("hlt");
    }
}
```

---

## Summary

### Key Takeaways

1. **VGA buffer is at 0xB8000** (physical, identity-mapped)
2. **Always use volatile** to prevent optimization
3. **16-bit format**: attribute (high byte) + character (low byte)
4. **Default attribute**: 0x07 (light gray on black)
5. **Testing**: QEMU shows VGA by default, use `-vnc` for headless

### Quick Reference Commands

```bash
# Build and test with VGA visible
zig build-obj -mcmodel=kernel src/main.zig -femit-bin=main.o
ld.lld-18 -T src/linker.ld -o iso/boot/kernel src/boot64.o main.o
grub-mkrescue -o bareflow.iso iso/
qemu-system-x86_64 -cdrom bareflow.iso

# Test with both VGA and serial
qemu-system-x86_64 -cdrom bareflow.iso -serial file:serial.log

# Capture screenshot
qemu-system-x86_64 -monitor stdio -cdrom bareflow.iso
# In monitor: screendump vga.ppm
```

### Files Involved

- **`src/main.zig`**: VGA implementation (lines 7, 103-118)
- **`src/boot64.S`**: Boot code (VGA available after long mode)
- **`src/linker.ld`**: Memory layout (0xB8000 identity-mapped)

---

**Last Updated**: 2025-11-01 (Session 47 - Option 2 Complete)
**References**:
- VGA Text Mode Wikipedia: https://en.wikipedia.org/wiki/VGA_text_mode
- OSDev VGA Text Mode: https://wiki.osdev.org/VGA_Text_Mode_Cursor
- Session 47 Implementation: commit 747f84e
