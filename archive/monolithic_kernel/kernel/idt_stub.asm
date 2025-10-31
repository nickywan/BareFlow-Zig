[BITS 32]

extern exception_handler
extern timer_handler
extern keyboard_handler

global default_isr
global isr_div_zero
global isr_debug
global isr_nmi
global isr_breakpoint
global isr_overflow
global isr_bound
global isr_invalid_op
global isr_gpf
global isr_page_fault
global irq_timer
global irq_keyboard

; Default ISR
default_isr:
    iret

; Exception handlers (0-13)
isr_div_zero:       ; #DE - Divide by zero
    push 0
    push 0
    jmp isr_common

isr_debug:          ; #DB - Debug
    push 0
    push 1
    jmp isr_common

isr_nmi:            ; NMI
    push 0
    push 2
    jmp isr_common

isr_breakpoint:     ; #BP - Breakpoint
    push 0
    push 3
    jmp isr_common

isr_overflow:       ; #OF - Overflow
    push 0
    push 4
    jmp isr_common

isr_bound:          ; #BR - Bound range
    push 0
    push 5
    jmp isr_common

isr_invalid_op:     ; #UD - Invalid opcode
    push 0
    push 6
    jmp isr_common

isr_gpf:            ; #GP - General protection fault
    push 13
    jmp isr_common

isr_page_fault:     ; #PF - Page fault
    push 14
    jmp isr_common

; Common ISR handler
isr_common:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10    ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call exception_handler  ; C function

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8      ; Remove error code and int number
    iret

; IRQ handlers
irq_timer:
    pusha
    call timer_handler
    popa
    iret

irq_keyboard:
    pusha
    call keyboard_handler
    popa
    iret
