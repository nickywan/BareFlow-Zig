/* Linker script for BareFlow Zig kernel */
ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 1MB */
    . = 1M;

    .text : ALIGN(4K)
    {
        __text_start = .;
        KEEP(*(.multiboot))
        *(.text)
        *(.text.*)
        __text_end = .;
    }

    .rodata : ALIGN(4K)
    {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        __rodata_end = .;
    }

    .data : ALIGN(4K)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    }

    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.comment)
        *(.note.*)
    }
}