/**
 * Linker Script for TinyLlama Unikernel
 *
 * Maps the kernel to physical address 0x1000 (4KB offset)
 * Bootloader jumps here after loading from disk sector 9+
 */

ENTRY(_start)

SECTIONS
{
    /* Kernel loaded at 4KB offset */
    . = 0x1000;

    /* Entry section MUST be first - contains FLUD signature at offset 0 */
    .text.entry : ALIGN(4) {
        *(.text.entry)
    }

    /* Code section */
    .text : ALIGN(4096) {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata : ALIGN(4096) {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data : ALIGN(4096) {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized data (zeroed by bootloader) */
    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* End of kernel marker */
    . = ALIGN(4096);
    __kernel_end = .;

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame)
        *(.gcc_except_table)
    }
}
