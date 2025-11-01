ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Multiboot header MUST be in first 32KB */
    .boot : ALIGN(4K)
    {
        *(.multiboot)
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .bss : ALIGN(4K)
    {
        *(.bss)
    }
}
