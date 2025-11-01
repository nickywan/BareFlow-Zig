# Simple boot test - just write to VGA and halt
# This tests if we can at least boot in 32-bit mode

.section .multiboot, "a"
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
.align 16
stack_bottom:
    .skip 16384                      # 16KB stack
stack_top:

.section .text
.code32
.global _start
.type _start, @function
_start:
    # Setup stack
    movl $stack_top, %esp

    # Write "OK!" to VGA
    movl $0xB8000, %edi
    movw $0x0F4F, (%edi)            # 'O' white on black
    movw $0x0F4B, 2(%edi)           # 'K' white on black
    movw $0x0F21, 4(%edi)           # '!' white on black

    # Halt forever
halt:
    cli
    hlt
    jmp halt

.size _start, . - _start
