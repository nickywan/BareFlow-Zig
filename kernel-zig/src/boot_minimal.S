# Ultra minimal multiboot2 kernel - just write to VGA and halt
# NO 64-bit transition, pure 32-bit

.section .multiboot, "a"
.align 8
multiboot_header:
    .long 0xE85250D6                # Multiboot2 magic
    .long 0                          # Architecture (i386)
    .long multiboot_header_end - multiboot_header
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    # End tag
    .align 8
    .short 0
    .short 0
    .long 8
multiboot_header_end:

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .text
.code32
.global _start
_start:
    # Write "BOOT!" to VGA at 0xB8000
    movl $0xB8000, %edi

    # Write each character with white-on-black attribute (0x0F)
    movw $0x0F42, (%edi)     # 'B'
    movw $0x0F4F, 2(%edi)    # 'O'
    movw $0x0F4F, 4(%edi)    # 'O'
    movw $0x0F54, 6(%edi)    # 'T'
    movw $0x0F21, 8(%edi)    # '!'

    # Halt forever
    cli
halt_loop:
    hlt
    jmp halt_loop
