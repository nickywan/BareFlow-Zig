; ============================================================================
; FLUID KERNEL - Entry Point (Assembly)
; ============================================================================
; File: kernel/entry.asm
; Description: Kernel entry point with signature
; This ensures the signature is EXACTLY at offset 0
; ============================================================================

[BITS 32]

section .text.entry

global _start
extern kernel_main

; ============================================================================
; ENTRY POINT - SIGNATURE MUST BE FIRST!
; ============================================================================
_start:
    ; Magic signature "FLUD" (0x464C5544)
    ; This MUST be at offset 0 of the kernel binary
    dd 0x464C5544
    
    ; Jump over signature to actual code
    jmp entry_point

; ============================================================================
; ACTUAL ENTRY POINT
; ============================================================================
entry_point:
    ; Setup stack (if not already done by bootloader)
    mov esp, 0x90000
    mov ebp, esp
    
    ; Clear direction flag (C convention)
    cld
    
    ; Call C kernel main
    call kernel_main
    
    ; If kernel_main returns (should never happen)
.hang:
    cli
    hlt
    jmp .hang