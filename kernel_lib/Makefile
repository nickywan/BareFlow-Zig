# Makefile for kernel_lib.a - Runtime Library
#
# Compiles all kernel_lib components into a single static library
# for linking with unikernel applications.

# ============================================================================
# TOOLCHAIN
# ============================================================================

CC = clang-18
AR = llvm-ar-18
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra \
         -fno-stack-protector -mno-sse

# ============================================================================
# SOURCE FILES
# ============================================================================

# I/O drivers
IO_SRCS = io/vga.c io/serial.c

# Memory management
MEM_SRCS = memory/malloc.c memory/string.c memory/compiler_rt.c

# CPU drivers
CPU_SRCS = cpu/pic.c cpu/idt.c

# JIT profiling
JIT_SRCS = jit/profile.c

# All sources
SRCS = $(IO_SRCS) $(MEM_SRCS) $(CPU_SRCS) $(JIT_SRCS)

# Object files
OBJS = $(SRCS:.c=.o)

# ============================================================================
# TARGETS
# ============================================================================

# Default target
all: kernel_lib.a

# Build static library
kernel_lib.a: $(OBJS)
	@echo "  [AR]  $@"
	@$(AR) rcs $@ $(OBJS)
	@echo "  [INFO] Library size:"
	@ls -lh $@ | awk '{print "        " $$5 " " $$9}'
	@if [ $$(stat -c%s $@) -gt 30720 ]; then \
		echo "  [WARN] Library exceeds 30KB target!"; \
	else \
		echo "  [OK]  Library within 30KB target"; \
	fi

# Compile .c to .o
%.o: %.c
	@echo "  [CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "  [CLEAN]"
	@rm -f $(OBJS) kernel_lib.a

# Rebuild everything
rebuild: clean all

# Show library info
info: kernel_lib.a
	@echo ""
	@echo "=== kernel_lib.a Info ==="
	@echo "Size: $$(stat -c%s kernel_lib.a) bytes"
	@echo "Symbols exported:"
	@nm kernel_lib.a | grep " T " | wc -l
	@echo ""
	@echo "Functions by module:"
	@echo "  I/O:    $$(nm kernel_lib.a | grep -E 'vga_|serial_|keyboard_' | wc -l)"
	@echo "  Memory: $$(nm kernel_lib.a | grep -E 'malloc|calloc|free|mem|str' | wc -l)"
	@echo "  CPU:    $$(nm kernel_lib.a | grep -E 'cpu_|pic_|idt_' | wc -l)"
	@echo "  JIT:    $$(nm kernel_lib.a | grep -E 'jit_' | wc -l)"
	@echo ""

.PHONY: all clean rebuild info
