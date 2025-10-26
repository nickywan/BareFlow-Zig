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
           -ffreestanding -nostdlib -fno-pie -O0 -Wall -Wextra \
           -fno-exceptions -fno-rtti \
           -fno-stack-protector -mno-red-zone -mcmodel=kernel \
           -fcf-protection=none \
           -DBARE_METAL \
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

kernel.o: kernel.c tinyllama_model.h
	@echo "  [CC]  $< (pure C kernel - O0 only working option)"
	@clang-18 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O0 -Wall -Wextra \
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none \
	          -I../../../kernel_lib -c $< -o $@

tinyllama_model.o: tinyllama_model.c tinyllama_model.h
	@echo "  [CC]  $< (O0 - O1/O2 break malloc)"
	@clang-18 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O0 \
	          -Wall -Wextra \
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none \
	          -I../../../kernel_lib -c $< -o $@

malloc_simple.o: malloc_simple.c
	@echo "  [CC]  $< (static heap override)"
	@clang-18 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none -c $< -o $@

serial.o: serial.c
	@echo "  [CC]  $< (serial I/O)"
	@clang-18 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none -c $< -o $@

tinyllama_inference.o: tinyllama_inference.c tinyllama_inference.h tinyllama_model.h
	@echo "  [CC]  $< (transformer inference - O0)"
	@clang-18 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O0 -Wall -Wextra \
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none \
	          -I../../../kernel_lib -c $< -o $@

$(KERNEL): boot.o kernel.o tinyllama_model.o tinyllama_inference.o malloc_simple.o serial.o
	@echo "  [LD]  $@ (standalone 64-bit, no kernel_lib)"
	@$(LD) $(LDFLAGS) boot.o kernel.o tinyllama_model.o tinyllama_inference.o malloc_simple.o serial.o -o $@
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
