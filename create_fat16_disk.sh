#!/bin/bash
# Create FAT16 disk image with bootloader and modules for BareFlow demo

set -e

echo "=== Creating BareFlow FAT16 Disk Image ==="

# Create 10MB disk image (larger than 1.44MB floppy for future LLVM)
dd if=/dev/zero of=bareflow_disk.img bs=1M count=10 2>/dev/null

# Format as FAT16
mkfs.fat -F 16 -n "BAREFLOW" bareflow_disk.img >/dev/null 2>&1

# Mount temporarily to copy files
mkdir -p /tmp/bareflow_mount
sudo mount -o loop bareflow_disk.img /tmp/bareflow_mount 2>/dev/null || {
    echo "Cannot mount with sudo, using mtools instead"

    # Copy modules using mtools (no sudo needed)
    if [ -d "modules" ]; then
        for mod in modules/*.mod; do
            if [ -f "$mod" ]; then
                mcopy -i bareflow_disk.img "$mod" ::
                echo "  Copied: $(basename $mod)"
            fi
        done
    fi

    # List files
    echo ""
    echo "=== Files on disk ==="
    mdir -i bareflow_disk.img ::

    # Unmount and write bootloader
    sudo umount /tmp/bareflow_mount 2>/dev/null || true
    rmdir /tmp/bareflow_mount 2>/dev/null || true

    # Write bootloader
    dd if=build/stage1.bin of=bareflow_disk.img bs=512 count=1 seek=0 conv=notrunc status=none
    dd if=build/stage2.bin of=bareflow_disk.img bs=512 count=8 seek=1 conv=notrunc status=none
    dd if=build/kernel.bin of=bareflow_disk.img bs=512 seek=9 conv=notrunc status=none

    echo ""
    echo "✓ Disk image created: bareflow_disk.img (10MB)"
    ls -lh bareflow_disk.img
    exit 0
}

# If mount succeeded with sudo
if [ -d "/tmp/bareflow_mount" ]; then
    # Copy all .mod files
    if [ -d "modules" ]; then
        sudo cp modules/*.mod /tmp/bareflow_mount/ 2>/dev/null || true
        echo "  Copied module files"
    fi

    # List files
    echo ""
    echo "=== Files on disk ==="
    ls -lh /tmp/bareflow_mount/

    # Unmount
    sudo umount /tmp/bareflow_mount
    rmdir /tmp/bareflow_mount
fi

# Write bootloader and kernel to the disk image
echo ""
echo "Writing bootloader..."
dd if=build/stage1.bin of=bareflow_disk.img bs=512 count=1 seek=0 conv=notrunc status=none
dd if=build/stage2.bin of=bareflow_disk.img bs=512 count=8 seek=1 conv=notrunc status=none
dd if=build/kernel.bin of=bareflow_disk.img bs=512 seek=9 conv=notrunc status=none

echo ""
echo "✓ Disk image created: bareflow_disk.img (10MB)"
ls -lh bareflow_disk.img
