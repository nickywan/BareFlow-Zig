# ============================================================================
# FLUID KERNEL - MAKEFILE
# ============================================================================

# Tools
ASM = nasm
CC = gcc
LD = ld
ASMFLAGS = -f bin

# Paths
BOOT_DIR = boot
KERNEL_DIR = kernel
BUILD_DIR = build

# Output
DISK_IMAGE = fluid.img
STAGE1_BIN = $(BUILD_DIR)/stage1.bin
STAGE2_BIN = $(BUILD_DIR)/stage2.bin
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

# Disk geometry
SECTOR_SIZE = 512
STAGE1_SECTORS = 1
STAGE2_SECTORS = 8
KERNEL_START_SECTOR = 9

# Colors for output
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m # No Color

.PHONY: all clean run debug rebuild info

all: info $(DISK_IMAGE)
	@echo "$(GREEN)✓ Build complete!$(NC)"

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build Stage 1 (512 bytes)
$(STAGE1_BIN): $(BOOT_DIR)/stage1.asm | $(BUILD_DIR)
	@echo "$(YELLOW)Building Stage 1...$(NC)"
	$(ASM) $(ASMFLAGS) $< -o $@
	@SIZE=$$(stat -f%z "$@" 2>/dev/null || stat -c%s "$@" 2>/dev/null); \
	if [ $$SIZE -ne 512 ]; then \
		echo "$(RED)ERROR: Stage 1 must be exactly 512 bytes (got $$SIZE)$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ Stage 1 built (512 bytes)$(NC)"

# Build Stage 2 (4KB = 8 sectors)
$(STAGE2_BIN): $(BOOT_DIR)/stage2.asm | $(BUILD_DIR)
	@echo "$(YELLOW)Building Stage 2...$(NC)"
	$(ASM) $(ASMFLAGS) $< -o $@
	@SIZE=$$(stat -f%z "$@" 2>/dev/null || stat -c%s "$@" 2>/dev/null); \
	EXPECTED=$$((512 * 8)); \
	if [ $$SIZE -ne $$EXPECTED ]; then \
		echo "$(RED)ERROR: Stage 2 must be exactly 4096 bytes (got $$SIZE)$(NC)"; \
		exit 1; \
	fi
	@echo "$(GREEN)✓ Stage 2 built (4096 bytes)$(NC)"

# Build Kernel (ASM entry + C code + stdlib + VGA + Module System)
$(KERNEL_BIN): $(KERNEL_DIR)/entry.asm $(KERNEL_DIR)/kernel.c $(KERNEL_DIR)/stdlib.c $(KERNEL_DIR)/vga.c $(KERNEL_DIR)/module_loader.c $(KERNEL_DIR)/linker.ld | $(BUILD_DIR)
	@echo "$(YELLOW)Building Kernel with Module System...$(NC)"
	# Assemble entry point
	$(ASM) -f elf32 $(KERNEL_DIR)/entry.asm -o $(BUILD_DIR)/entry.o

	# Compile VGA driver
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
		-c $(KERNEL_DIR)/vga.c -o $(BUILD_DIR)/vga.o

	# Compile module loader
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
		-c $(KERNEL_DIR)/module_loader.c -o $(BUILD_DIR)/module_loader.o

	# Compile kernel C code
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
		-c $(KERNEL_DIR)/kernel.c -o $(BUILD_DIR)/kernel.o

	# Compile stdlib
	$(CC) -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
		-c $(KERNEL_DIR)/stdlib.c -o $(BUILD_DIR)/stdlib.o

	# Link everything (ORDRE IMPORTANT)
	$(LD) -m elf_i386 -T $(KERNEL_DIR)/linker.ld --oformat binary \
		$(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/module_loader.o \
		$(BUILD_DIR)/vga.o $(BUILD_DIR)/stdlib.o -o $@
	
	@SIZE=$$(stat -f%z "$@" 2>/dev/null || stat -c%s "$@" 2>/dev/null); \
	echo "$(GREEN)✓ Kernel built ($$SIZE bytes)$(NC)"
	@echo "$(YELLOW)Verifying signature...$(NC)"
	@SIG=$$(hexdump -n 4 -e '4/1 "%02x"' $@); \
	if [ "$$SIG" = "44554c46" ]; then \
		echo "$(GREEN)✓ Signature OK: FLUD (44554c46)$(NC)"; \
	else \
		echo "$(RED)✗ Invalid signature: 0x$$SIG$(NC)"; \
		exit 1; \
	fi

# Create bootable disk image
$(DISK_IMAGE): $(STAGE1_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "$(YELLOW)Creating disk image...$(NC)"
	# Create empty 1.44MB floppy image
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	
	# Write Stage 1 at sector 0 (boot sector)
	dd if=$(STAGE1_BIN) of=$@ bs=512 count=1 seek=0 conv=notrunc status=none
	
	# Write Stage 2 at sector 1 (right after boot sector)
	dd if=$(STAGE2_BIN) of=$@ bs=512 count=8 seek=1 conv=notrunc status=none
	
	# Write Kernel at sector 9 (after stage1 + stage2)
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=$(KERNEL_START_SECTOR) conv=notrunc status=none
	
	@echo "$(GREEN)✓ Disk image created: $(DISK_IMAGE)$(NC)"

# Show disk layout
info:
	@echo "$(YELLOW)=== Fluid Bootloader Disk Layout ===$(NC)"
	@echo "Sector 0:       Stage 1 (bootloader)"
	@echo "Sectors 1-8:    Stage 2 (extended bootloader)"
	@echo "Sectors 9+:     Kernel"
	@echo ""

# Run in QEMU
run: $(DISK_IMAGE)
	@echo "$(GREEN)Starting QEMU...$(NC)"
	qemu-system-i386 -drive file=$(DISK_IMAGE),format=raw -serial stdio

# Debug with QEMU
debug: $(DISK_IMAGE)
	@echo "$(GREEN)Starting QEMU in debug mode...$(NC)"
	qemu-system-i386 -drive file=$(DISK_IMAGE),format=raw -serial stdio -d int,cpu_reset -no-reboot

# Clean build files
clean:
	@echo "$(RED)Cleaning build files...$(NC)"
	rm -rf $(BUILD_DIR)
	rm -f $(DISK_IMAGE)
	@echo "$(GREEN)✓ Clean complete$(NC)"

# Rebuild from scratch
rebuild: clean all

# Help
help:
	@echo "Fluid Kernel Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build everything (default)"
	@echo "  run     - Build and run in QEMU"
	@echo "  debug   - Build and run with debug output"
	@echo "  clean   - Remove all build files"
	@echo "  rebuild - Clean and build"
	@echo "  info    - Show disk layout"
	@echo "  help    - Show this help"