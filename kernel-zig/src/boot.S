# Multiboot2 header and boot entry for x86_64
# GRUB boots us in 32-bit protected mode, so we need to setup long mode

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

# Page tables in .data (must NOT be in BSS or they get zeroed!)
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
.code32
.global _start
.type _start, @function
_start:
    # Disable interrupts IMMEDIATELY
    cli

    # Setup stack FIRST (before any function calls)
    movl $stack_top, %esp
    xorl %ebp, %ebp

    # Write '1' to VGA
    movl $0xB8000, %edi
    movw $0x0F31, (%edi)

    # Disable PIC - mask all interrupts
    movb $0xFF, %al
    outb %al, $0xA1                 # Mask all IRQs on slave PIC
    outb %al, $0x21                 # Mask all IRQs on master PIC

    # Zero BSS early (before any memory usage)
    call zero_bss

    # Write '2' to VGA
    movl $0xB8000, %edi
    movw $0x0F32, 2(%edi)

    # Save multiboot info (after BSS is zeroed)
    pushl %ebx
    pushl %eax

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

# Zero BSS in 32-bit mode (before any memory usage!)
.code32
zero_bss:
    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb
    ret

# Now we're in 64-bit long mode!
.code64
long_mode_start:
    # Disable interrupts (again, just to be safe)
    cli

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

    # TODO: Setup IDT in Zig code if needed
    # For now, we're running with interrupts disabled

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
