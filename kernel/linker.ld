OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    . = 0x1000;
    
    /* Entry point with signature MUST be first */
    .text.entry : {
        *(.text.entry)
    }
    
    /* Rest of code */
    .text : {
        *(.text)
    }
    
    /* Read-only data */
    .rodata : {
        *(.rodata*)
    }
    
    /* Initialized data */
    .data : {
        *(.data)
    }
    
    /* Uninitialized data */
    .bss : {
        *(.bss)
        *(COMMON)
    }
}