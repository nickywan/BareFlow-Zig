/**
 * Multiboot2 Boot Entry for 64-bit Kernel with LLVM
 */

.section .multiboot
.align 8

multiboot_header_start:
    .long 0xE85250D6                    // Multiboot2 magic
    .long 0                             // Architecture: i386
    .long multiboot_header_end - multiboot_header_start  // Header length
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))  // Checksum

    // End tag
    .short 0                            // type
    .short 0                            // flags
    .long 8                             // size
multiboot_header_end:

.section .bss
.align 16
stack_bottom:
    .skip 16384                         // 16 KB stack
stack_top:

.section .text
.global _start
.type _start, @function

_start:
    // Setup stack
    mov $stack_top, %rsp

    // Clear direction flag
    cld

    // Zero .bss section (CRITICAL for malloc)
    movabs $_bss_start, %rdi
    movabs $_bss_end, %rcx
    sub %rdi, %rcx          // size = end - start
    xor %eax, %eax          // value = 0
    rep stosb               // memset(bss, 0, size)

    // Call kernel main
    call kernel_main

    // Halt if kernel returns
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
