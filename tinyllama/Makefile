# Makefile for TinyLlama Unikernel
#
# Builds a self-profiling bare-metal application using kernel_lib.a
#
# Target: tinyllama_bare.bin (~100KB max)

# ============================================================================
# TOOLCHAIN
# ============================================================================

CC = clang-18
LD = ld
ASM = nasm
OBJCOPY = objcopy

# Compiler flags for bare-metal 32-bit
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
         -fno-stack-protector -mno-sse -I../kernel_lib

# Assembler flags
ASMFLAGS = -f elf32

# Linker flags
LDFLAGS = -m elf_i386 -T linker.ld --oformat elf32-i386

# ============================================================================
# PATHS
# ============================================================================

KERNEL_LIB = ../kernel_lib/kernel_lib.a
BOOTLOADER_STAGE1 = ../build/stage1.bin
BOOTLOADER_STAGE2 = ../build/stage2.bin

# ============================================================================
# BUILD TARGETS
# ============================================================================

# Object files
OBJS = entry.o main.o

# Output files
ELF = tinyllama_bare.elf
BIN = tinyllama_bare.bin
IMG = tinyllama.img

# ============================================================================
# DEFAULT TARGET
# ============================================================================

all: $(BIN)
	@echo ""
	@echo "=== TinyLlama Build Complete ==="
	@echo "Binary size: $$(stat -c%s $(BIN)) bytes"
	@if [ $$(stat -c%s $(BIN)) -gt 102400 ]; then \
		echo "WARNING: Binary exceeds 100KB target!"; \
	else \
		echo "OK: Binary within 100KB target"; \
	fi
	@echo ""

# ============================================================================
# COMPILATION
# ============================================================================

# Assemble entry point
entry.o: entry.asm
	@echo "  [ASM]  $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

# Compile main
main.o: main.c
	@echo "  [CC]   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# LINKING
# ============================================================================

# Link ELF executable
$(ELF): $(OBJS) $(KERNEL_LIB) linker.ld
	@echo "  [LD]   $@"
	@$(LD) $(LDFLAGS) $(OBJS) $(KERNEL_LIB) -o $@
	@echo "  [INFO] ELF size: $$(stat -c%s $@) bytes"

# Extract raw binary
$(BIN): $(ELF)
	@echo "  [BIN]  $@"
	@$(OBJCOPY) -O binary $< $@
	@echo "  [INFO] Verifying FLUD signature..."
	@if hexdump -n 4 -e '4/1 "%02x"' $@ | grep -q "44554c46"; then \
		echo "  [OK]   FLUD signature (little-endian) found at offset 0"; \
	else \
		echo "  [ERROR] Missing FLUD signature!"; \
		exit 1; \
	fi

# ============================================================================
# BOOTABLE IMAGE
# ============================================================================

$(IMG): $(BIN)
	@echo "  [IMG]  Creating bootable image..."
	@if [ ! -f $(BOOTLOADER_STAGE1) ]; then \
		echo "  [ERROR] Stage 1 bootloader not found. Run: make -C ../boot"; \
		exit 1; \
	fi
	@if [ ! -f $(BOOTLOADER_STAGE2) ]; then \
		echo "  [ERROR] Stage 2 bootloader not found. Run: make -C ../boot"; \
		exit 1; \
	fi
	@# Create 10MB image
	@dd if=/dev/zero of=$@ bs=512 count=20480 2>/dev/null
	@# Write stage 1 (sector 0)
	@dd if=$(BOOTLOADER_STAGE1) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	@# Write stage 2 (sectors 1-8)
	@dd if=$(BOOTLOADER_STAGE2) of=$@ bs=512 seek=1 count=8 conv=notrunc 2>/dev/null
	@# Write kernel (sector 9+)
	@dd if=$(BIN) of=$@ bs=512 seek=9 conv=notrunc 2>/dev/null
	@echo "  [OK]   Bootable image ready: $@"
	@echo "         Image size: $$(stat -c%s $@) bytes"

# ============================================================================
# RUN IN QEMU
# ============================================================================

run: $(IMG)
	@echo "  [QEMU] Launching TinyLlama..."
	@qemu-system-i386 -drive file=$(IMG),format=raw -serial stdio

debug: $(IMG)
	@echo "  [QEMU] Launching with debug output..."
	@qemu-system-i386 -drive file=$(IMG),format=raw -serial stdio -d cpu_reset,int

# ============================================================================
# UTILITY TARGETS
# ============================================================================

clean:
	@echo "  [CLEAN]"
	@rm -f $(OBJS) $(ELF) $(BIN) $(IMG)

rebuild: clean all

info: $(BIN)
	@echo ""
	@echo "=== TinyLlama Binary Info ==="
	@echo "Size: $$(stat -c%s $(BIN)) bytes ($$(echo "scale=1; $$(stat -c%s $(BIN))/1024" | bc)KB)"
	@echo ""
	@echo "Sections from ELF:"
	@readelf -S $(ELF) | grep -E '\.text|\.rodata|\.data|\.bss'
	@echo ""
	@echo "Top 10 symbols by size:"
	@nm -S --size-sort $(ELF) | tail -10
	@echo ""

.PHONY: all run debug clean rebuild info
