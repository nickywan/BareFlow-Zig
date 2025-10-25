; ============================================================================
; TinyLlama Unikernel - Entry Point
; ============================================================================
; Entry point with FLUD signature for bootloader validation
; ============================================================================

[BITS 32]

section .text.entry

global _start
extern main

; ============================================================================
; ENTRY POINT - SIGNATURE MUST BE FIRST!
; ============================================================================
_start:
    ; Magic signature "FLUD" (0x464C5544)
    ; This MUST be at offset 0 of the binary
    dd 0x464C5544

    ; Jump over signature to actual code
    jmp entry_point

; ============================================================================
; ACTUAL ENTRY POINT
; ============================================================================
entry_point:
    ; Setup stack
    mov esp, 0x90000
    mov ebp, esp

    ; Clear direction flag (C convention)
    cld

    ; Call C main
    call main

    ; If main returns (infinite loop)
.hang:
    cli
    hlt
    jmp .hang
