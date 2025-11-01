# Multiboot2 header and boot entry for x86_64
# GRUB boots us in 32-bit protected mode, so we need to setup long mode

.section .multiboot
.align 8
multiboot_header:
    .long 0xE85250D6                # Multiboot2 magic
    .long 0                          # Architecture (i386 protected mode)
    .long multiboot_header_end - multiboot_header
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    # End tag
    .align 8
    .word 0
    .word 0
    .long 8
multiboot_header_end:

.section .bss
.align 4096
# Page tables for 64-bit mode
pml4:
    .skip 4096
pdpt:
    .skip 4096
pd:
    .skip 4096

.align 16
stack_bottom:
    .skip 65536                      # 64KB stack
stack_top:

.section .text
.code32
.global _start
.type _start, @function
_start:
    # We're in 32-bit protected mode
    # Write '1' to VGA to show we booted
    movl $0xB8000, %edi
    movw $0x0F31, (%edi)           # '1' in white on black

    # Setup temporary stack
    movl $stack_top, %esp

    # Clear frame pointer
    xorl %ebp, %ebp

    # Save multiboot info
    pushl %ebx
    pushl %eax

    # Write '2' to VGA
    movw $0x0F32, 2(%edi)

    # Setup page tables for 64-bit long mode
    call setup_page_tables

    # Write '3' to VGA
    movl $0xB8000, %edi
    movw $0x0F33, 4(%edi)

    # Enable PAE
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    # Write '4' to VGA
    movw $0x0F34, 6(%edi)

    # Load PML4
    movl $pml4, %eax
    movl %eax, %cr3

    # Enable long mode in EFER MSR
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax             # Set LME bit
    wrmsr

    # Enable paging and enter long mode
    movl %cr0, %eax
    orl $(1 << 31), %eax            # Set PG bit
    movl %eax, %cr0

    # Load 64-bit GDT
    lgdt gdt64_pointer

    # Far jump to 64-bit code
    ljmp $0x08, $long_mode_start

# Setup identity-mapped page tables
.code32
setup_page_tables:
    # Zero out page tables
    movl $pml4, %edi
    movl $0, %eax
    movl $3072, %ecx                # 3 pages * 1024 dwords
    rep stosl

    # PML4[0] -> PDPT
    movl $pdpt, %eax
    orl $3, %eax                    # Present + writable
    movl %eax, pml4

    # PDPT[0] -> PD
    movl $pd, %eax
    orl $3, %eax
    movl %eax, pdpt

    # Map first 2MB with 2MB pages in PD
    movl $pd, %edi
    movl $0x83, %eax                # Present + writable + huge (2MB)
    movl $512, %ecx                 # 512 entries = 1GB
1:
    movl %eax, (%edi)
    addl $0x200000, %eax            # Next 2MB
    addl $8, %edi
    loop 1b

    ret

# Now we're in 64-bit long mode!
.code64
long_mode_start:
    # Set up segment registers
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Setup 64-bit stack
    movq $stack_top, %rsp
    xorq %rbp, %rbp

    # Zero BSS section
    movq $__bss_start, %rdi
    movq $__bss_end, %rcx
    subq %rdi, %rcx
    xorq %rax, %rax
    rep stosb

    # Call Zig kernel main
    call kernel_main

    # If kernel_main returns, halt
halt:
    cli
    hlt
    jmp halt

.size _start, . - _start

# 64-bit GDT
.section .rodata
.align 16
gdt64:
    .quad 0                         # Null descriptor
    .quad 0x00AF9A000000FFFF        # Code segment (64-bit)
    .quad 0x00CF92000000FFFF        # Data segment

gdt64_pointer:
    .word gdt64_pointer - gdt64 - 1
    .quad gdt64
