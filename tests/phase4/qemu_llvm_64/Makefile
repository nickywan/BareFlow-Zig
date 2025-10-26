# Makefile for BareFlow QEMU x86-64 Kernel with LLVM
#
# Builds a 64-bit kernel that boots via GRUB/Multiboot2

# ============================================================================
# Configuration
# ============================================================================

KERNEL = kernel.elf
ISO = bareflow.iso

KERNEL_LIB = ../../../kernel_lib/kernel_lib_llvm.a

# ============================================================================
# Toolchain
# ============================================================================

AS = clang-18
CC = clang++-18
LD = ld.lld-18

# Assembly flags
ASFLAGS = -target x86_64-unknown-none

# C++ flags for 64-bit bare-metal
CXXFLAGS = -target x86_64-unknown-none \
           -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
           -fno-exceptions -fno-rtti \
           -fno-stack-protector -mno-red-zone -mcmodel=kernel \
           -DHEAP_SIZE_SMALL \
           -I../../../kernel_lib

# Linker flags
LDFLAGS = -T linker.ld -nostdlib

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean run iso

all: $(KERNEL)

boot.o: boot.S
	@echo "  [AS]  $<"
	@$(AS) $(ASFLAGS) -c $< -o $@

kernel.o: kernel.cpp
	@echo "  [CXX] $<"
	@$(CC) $(CXXFLAGS) -c $< -o $@

$(KERNEL): boot.o kernel.o $(KERNEL_LIB)
	@echo "  [LD]  $@"
	@$(LD) $(LDFLAGS) boot.o kernel.o $(KERNEL_LIB) -o $@
	@echo "  [INFO] Kernel size: $$(stat -c%s $@) bytes"

iso: $(ISO)

$(ISO): $(KERNEL)
	@echo "  [ISO] Creating bootable ISO..."
	@mkdir -p isodir/boot/grub
	@cp $(KERNEL) isodir/boot/kernel.elf
	@echo 'set timeout=0' > isodir/boot/grub/grub.cfg
	@echo 'set default=0' >> isodir/boot/grub/grub.cfg
	@echo '' >> isodir/boot/grub/grub.cfg
	@echo 'menuentry "BareFlow QEMU x86-64" {' >> isodir/boot/grub/grub.cfg
	@echo '    multiboot2 /boot/kernel.elf' >> isodir/boot/grub/grub.cfg
	@echo '    boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@grub-mkrescue -o $@ isodir 2>/dev/null
	@echo "  [OK]  ISO created: $@"

run: $(ISO)
	@echo ""
	@echo "========================================="
	@echo "  Launching BareFlow in QEMU x86-64"
	@echo "========================================="
	@echo ""
	@qemu-system-x86_64 -cdrom $(ISO) -serial stdio -m 512M

clean:
	@echo "  [CLEAN]"
	@rm -f *.o $(KERNEL) $(ISO)
	@rm -rf isodir

.PHONY: all clean run iso
