#!/bin/bash
# Debug script for BareFlow Zig kernel

echo "Starting QEMU with GDB server..."
echo "Connect with: gdb iso/boot/kernel -ex 'target remote :1234'"
echo ""

qemu-system-x86_64 \
    -M q35 \
    -m 128 \
    -cdrom test-minimal.iso \
    -s \
    -S \
    -serial stdio \
    -d int,cpu_reset \
    -D qemu-gdb.log
