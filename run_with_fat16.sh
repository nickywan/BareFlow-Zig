#!/bin/bash
# Run BareFlow kernel with FAT16 test disk attached

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Starting BareFlow with FAT16 test disk...${NC}"

# Check if FAT16 disk exists
if [ ! -f "build/fat16_test.img" ]; then
    echo -e "${YELLOW}FAT16 test disk not found, creating it...${NC}"
    python3 tools/create_fat16_disk.py
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to create FAT16 disk"
        exit 1
    fi
fi

echo -e "${GREEN}Launching QEMU with two drives:${NC}"
echo "  Drive 0: fluid.img (boot disk)"
echo "  Drive 1: build/fat16_test.img (FAT16 data disk)"
echo ""

# Kill any existing background shell that might be using the serial log
pkill -f "make run" 2>/dev/null

# Clear serial log
> /tmp/serial.log

# Run QEMU with both disks
qemu-system-i386 \
    -drive format=raw,file=fluid.img,index=0,media=disk \
    -drive format=raw,file=build/fat16_test.img,index=1,media=disk \
    -serial file:/tmp/serial.log \
    -m 32M

echo ""
echo -e "${GREEN}Serial output saved to: /tmp/serial.log${NC}"
