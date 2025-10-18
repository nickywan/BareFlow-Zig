; ============================================================================
; FLUID KERNEL - BOOTLOADER STAGE 1
; ============================================================================
; File: boot/stage1.asm
; Description: First stage bootloader - loads stage 2
; Size: 512 bytes (1 sector)
; Load address: 0x7C00 (loaded by BIOS)
; ============================================================================

[BITS 16]
[ORG 0x7C00]

; === CONSTANTS ===
STAGE2_OFFSET equ 0x7E00        ; Load stage 2 right after stage 1
STAGE2_SECTORS equ 8            ; Stage 2 size (4KB = 8 sectors)

; ============================================================================
; ENTRY POINT
; ============================================================================
start:
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; Stack below bootloader
    sti
    
    ; Save boot drive
    mov [BOOT_DRIVE], dl
    
    ; Print loading message
    mov si, msg_stage1
    call print_string
    
    ; Load stage 2
    call load_stage2
    
    ; Print success
    mov si, msg_loaded
    call print_string
    
    ; Jump to stage 2
    mov dl, [BOOT_DRIVE]        ; Pass boot drive to stage 2
    jmp STAGE2_OFFSET

; ============================================================================
; FUNCTION: load_stage2
; ============================================================================
; Description: Load stage 2 bootloader from disk
; ============================================================================
load_stage2:
    mov cx, 3                   ; Try 3 times
    
.retry:
    push cx
    
    ; Reset disk
    xor ah, ah
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    ; Read stage 2 (CHS method for simplicity in stage 1)
    mov ah, 0x02                ; Read sectors
    mov al, STAGE2_SECTORS      ; Number of sectors
    mov ch, 0x00                ; Cylinder 0
    mov cl, 0x02                ; Start at sector 2 (sector 1 is stage 1)
    mov dh, 0x00                ; Head 0
    mov dl, [BOOT_DRIVE]
    mov bx, STAGE2_OFFSET       ; Destination
    int 0x13
    
    pop cx
    jnc .success                ; No error, done
    
    loop .retry                 ; Retry
    
    ; Failed after all retries
    mov si, msg_error
    call print_string
    jmp .hang
    
.success:
    ret

.hang:
    cli
    hlt
    jmp .hang

; ============================================================================
; FUNCTION: print_string
; ============================================================================
; Description: Print null-terminated string
; Input: SI = pointer to string
; ============================================================================
print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp .loop
.done:
    popa
    ret

; ============================================================================
; DATA
; ============================================================================
BOOT_DRIVE:     db 0

msg_stage1:     db 'Fluid Stage1', 13, 10, 0
msg_loaded:     db 'Stage2 loaded', 13, 10, 0
msg_error:      db 'LOAD ERROR!', 13, 10, 0

; ============================================================================
; PADDING AND BOOT SIGNATURE
; ============================================================================
times 510-($-$$) db 0
dw 0xAA55