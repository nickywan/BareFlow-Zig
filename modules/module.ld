/* Linker script for bare-metal modules
 * Ensures .module_header is at offset 0, followed immediately by .text
 */

SECTIONS
{
    /* Module header must be first, at offset 0 */
    .module_header 0x0 : {
        *(.module_header)
    }

    /* Code section follows immediately after header
     * sizeof(module_header_t) = 48 bytes = 0x30
     */
    .text 0x30 : {
        *(.text .text.*)
    }

    /* Optional: read-only data */
    .rodata : {
        *(.rodata .rodata.*)
    }

    /* Discard unused sections */
    /DISCARD/ : {
        *(.comment)
        *(.note.GNU-stack)
        *(.llvm_addrsig)
        *(.eh_frame)
    }
}
