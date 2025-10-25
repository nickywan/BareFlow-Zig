#!/bin/bash
# Run BareFlow kernel in INTERACTIVE mode with FAT16 test disk
# This will show VGA output in QEMU window with keyboard pauses

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  BareFlow FAT16 Test (Interactive)${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""
echo -e "${YELLOW}Mode: INTERACTIVE (with keyboard pauses)${NC}"
echo -e "${GREEN}Drives:${NC}"
echo "  • Drive 0: fluid.img (boot disk)"
echo "  • Drive 1: build/fat16_test.img (FAT16 data disk)"
echo ""
echo -e "${YELLOW}Instructions:${NC}"
echo "  • Watch for 'Press any key' prompts"
echo "  • Press any key in QEMU window to continue through tests"
echo "  • FAT16 test will show filesystem info and file listing"
echo ""
echo -e "${GREEN}Starting QEMU...${NC}"
echo ""

# Check if FAT16 disk exists
if [ ! -f "build/fat16_test.img" ]; then
    echo -e "${YELLOW}FAT16 test disk not found, creating it...${NC}"
    python3 tools/create_fat16_disk.py
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to create FAT16 disk"
        exit 1
    fi
    echo ""
fi

# Run QEMU with graphical display
qemu-system-i386 \
    -drive format=raw,file=fluid.img,index=0,media=disk \
    -drive format=raw,file=build/fat16_test.img,index=1,media=disk \
    -m 64M

echo ""
echo -e "${GREEN}✓ QEMU session ended${NC}"
