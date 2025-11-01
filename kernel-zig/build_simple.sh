#!/bin/bash
# Build script for bareflow-simple.iso
# Uses manual compilation with -mcmodel=kernel flag for proper 64-bit addressing
#
# Session 47: This fixes the bug where Zig generated 32-bit pointer loads (EDI)
# instead of 64-bit (RDI) for string literals, causing infinite 'E' characters.
#
# With -mcmodel=kernel, Zig completely inlines serial_print(), resulting in
# perfect output with no pointer addressing issues!

set -e  # Exit on error

echo "=== Building BareFlow-Zig Kernel (Simple) ==="

# Ensure PATH includes Zig
export PATH=/tmp/zig-linux-x86_64-0.13.0:/usr/bin:/bin:$PATH

# 1. Compile main_simple.zig with kernel code model
echo "Compiling main_simple.zig with -mcmodel=kernel..."
zig build-obj -target x86_64-freestanding -mcmodel=kernel -O ReleaseSafe \
    src/main_simple.zig -femit-bin=main_simple.o

# 2. Compile boot64.S
echo "Compiling boot64.S..."
gcc -c -m64 -mno-red-zone -mcmodel=kernel -fno-pie \
    src/boot64.S -o src/boot64.o

# 3. Link kernel
echo "Linking kernel..."
ld.lld-18 -T src/linker.ld -o iso/boot/kernel src/boot64.o main_simple.o

# 4. Create ISO
echo "Creating ISO..."
grub-mkrescue -o bareflow-simple.iso iso/ 2>&1 | grep -E "ISO image produced|Written to medium"

# 5. Test in QEMU
echo ""
echo "=== Testing in QEMU ==="
timeout 3 qemu-system-x86_64 -M q35 -m 128 -cdrom bareflow-simple.iso \
    -serial file:serial-test.log || true

# 6. Show results
echo ""
echo "=== Serial Output ==="
cat serial-test.log
echo ""
echo "=== Byte Count ==="
wc -c serial-test.log
echo ""
echo "SUCCESS! Build complete: bareflow-simple.iso"
