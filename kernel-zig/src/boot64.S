# Multiboot2 header and boot entry for x86_64
# Pure 64-bit mode - NO 32-bit transition needed
# GRUB loads us directly in 64-bit long mode

.section .multiboot, "a"
.align 8
multiboot_header:
    .long 0xE85250D6                # Multiboot2 magic
    .long 0                          # Architecture (i386 protected mode)
    .long multiboot_header_end - multiboot_header
    # Checksum
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    # End tag (type=0, flags=0, size=8) - MUST be 8-byte aligned
    .align 8
    .short 0                         # type
    .short 0                         # flags
    .long 8                          # size
multiboot_header_end:

# Page tables in .data (pre-initialized)
.section .data
.align 4096
pml4:
    .skip 4096
pdpt:
    .skip 4096
pd:
    .skip 4096

.section .bss
.align 16
stack_bottom:
    .skip 65536                      # 64KB stack
stack_top:

.section .text
.code64                              # We're already in 64-bit!
.global _start
.type _start, @function
_start:
    # Setup stack (64-bit)
    mov $stack_top, %rsp
    xor %rbp, %rbp

    # Clear direction flag
    cld

    # Zero BSS section FIRST
    movabs $__bss_start, %rdi
    movabs $__bss_end, %rcx
    sub %rdi, %rcx
    xor %eax, %eax
    rep stosb

    # Setup page tables for identity mapping
    # PML4[0] -> PDPT
    movabs $pml4, %rdi
    movabs $pdpt, %rax
    or $0x003, %rax                 # Present + Writable
    mov %rax, (%rdi)

    # PDPT[0] -> PD
    movabs $pdpt, %rdi
    movabs $pd, %rax
    or $0x003, %rax
    mov %rax, (%rdi)

    # PD: Map first 1GB with 2MB pages
    movabs $pd, %rdi
    mov $512, %rcx                   # 512 entries = 1GB
    xor %rax, %rax
1:
    mov %rax, %rdx
    or $0x083, %rdx                  # Present + Writable + Huge (2MB)
    mov %rdx, (%rdi)
    add $(2 * 1024 * 1024), %rax    # Next 2MB
    add $8, %rdi
    loop 1b

    # Load page tables
    movabs $pml4, %rax
    mov %rax, %cr3

    # Call Zig kernel main
    call kernel_main

    # If kernel_main returns, halt
halt:
    cli
    hlt
    jmp halt

.size _start, . - _start