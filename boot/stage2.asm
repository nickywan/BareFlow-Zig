; ============================================================================
; FLUID KERNEL - BOOTLOADER STAGE 2
; ============================================================================
; File: boot/stage2.asm
; Description: Second stage bootloader - full featured
; Size: ~4KB (8 sectors)
; Load address: 0x7E00
; ============================================================================

[BITS 16]
[ORG 0x7E00]

; === CONSTANTS ===
KERNEL_OFFSET equ 0x1000
KERNEL_SECTORS equ 48              ; 48 sectors = 24KB (kernel is ~19KB)
KERNEL_SIGNATURE equ 0x464C5544    ; "FLUD"
MAX_RETRIES equ 3

; Error codes
ERR_DISK_READ equ 0xD1
ERR_DISK_VERIFY equ 0xD2
ERR_A20_FAILED equ 0xA2
ERR_BAD_SIGNATURE equ 0x51
ERR_NO_KERNEL equ 0xE0

; ============================================================================
; ENTRY POINT
; ============================================================================
stage2_start:
    ; DL contains boot drive from stage 1
    mov [BOOT_DRIVE], dl
    
    ; Print stage 2 banner
    mov si, msg_banner
    call print_string
    
    ; Check LBA support
    mov si, msg_check_lba
    call print_string
    call check_lba_support
    mov [lba_available], al
    
    cmp al, 1
    je .lba_yes
    mov si, msg_lba_no
    jmp .print_lba
.lba_yes:
    mov si, msg_lba_yes
.print_lba:
    call print_string
    
    ; Enable A20
    mov si, msg_enable_a20
    call print_string
    call enable_a20
    call verify_a20
    jc .a20_error
    
    mov si, msg_ok
    call print_string
    
    ; Load kernel
    mov si, msg_loading_kernel
    call print_string
    call load_kernel
    
    mov si, msg_ok
    call print_string
    
    ; Verify kernel signature
    mov si, msg_verify_sig
    call print_string
    
    mov eax, [KERNEL_OFFSET]
    cmp eax, KERNEL_SIGNATURE
    jne .sig_error
    
    mov si, msg_ok
    call print_string
    
    ; All good, enter protected mode
    mov si, msg_enter_pm
    call print_string
    
    ; Small delay to read message
    mov cx, 0x8000
.delay:
    nop
    loop .delay
    
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:protected_mode_start

; Error handlers
.a20_error:
    mov al, ERR_A20_FAILED
    jmp fatal_error

.sig_error:
    mov al, ERR_BAD_SIGNATURE
    jmp fatal_error

; ============================================================================
; FUNCTION: check_lba_support
; ============================================================================
check_lba_support:
    push bx
    push cx
    push dx
    
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    jc .no_lba
    cmp bx, 0xAA55
    jne .no_lba
    
    mov al, 1
    jmp .done
    
.no_lba:
    mov al, 0
    
.done:
    pop dx
    pop cx
    pop bx
    ret

; ============================================================================
; FUNCTION: load_kernel
; ============================================================================
load_kernel:
    mov byte [retry_count], 0
    
.retry_loop:
    ; Reset disk
    xor ah, ah
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    ; Try LBA if available
    cmp byte [lba_available], 1
    je .try_lba
    jmp .use_chs
    
.try_lba:
    call load_kernel_lba
    jnc .success
    ; LBA failed, try CHS
    
.use_chs:
    call load_kernel_chs
    jnc .success
    
    ; Both failed, retry
    inc byte [retry_count]
    cmp byte [retry_count], MAX_RETRIES
    jl .retry_loop
    
    ; All retries exhausted
    mov al, ERR_DISK_READ
    jmp fatal_error
    
.success:
    ret

; ============================================================================
; FUNCTION: load_kernel_lba
; ============================================================================
load_kernel_lba:
    ; Setup DAP (simple, single read)
    mov byte [dap_size], 0x10
    mov byte [dap_reserved], 0
    mov word [dap_sectors], KERNEL_SECTORS
    mov word [dap_offset], KERNEL_OFFSET
    mov word [dap_segment], 0
    mov dword [dap_lba_low], 9
    mov dword [dap_lba_high], 0

    mov si, dap_size
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13

    ret

; ============================================================================
; FUNCTION: load_kernel_chs
; ============================================================================
load_kernel_chs:
    mov ah, 0x02                ; Read
    mov al, KERNEL_SECTORS      ; Sector count
    mov ch, 0x00                ; Cylinder 0
    mov cl, 0x0A                ; Sector 10 (9+1, CHS is 1-indexed)
    mov dh, 0x00                ; Head 0
    mov dl, [BOOT_DRIVE]
    mov bx, KERNEL_OFFSET       ; Destination
    int 0x13

    jc .error

    ; Verify count (AL contains sectors read)
    cmp al, KERNEL_SECTORS
    jne .verify_error
    clc
    ret

.verify_error:
    mov al, ERR_DISK_VERIFY
    jmp fatal_error

.error:
    stc
    ret

; ============================================================================
; FUNCTION: enable_a20
; ============================================================================
enable_a20:
    ; Method 1: BIOS
    mov ax, 0x2401
    int 0x15
    jnc .done
    
    ; Method 2: Keyboard controller
    call .wait_kbd
    mov al, 0xAD
    out 0x64, al
    
    call .wait_kbd
    mov al, 0xD0
    out 0x64, al
    
    call .wait_data
    in al, 0x60
    push ax
    
    call .wait_kbd
    mov al, 0xD1
    out 0x64, al
    
    call .wait_kbd
    pop ax
    or al, 2
    out 0x60, al
    
    call .wait_kbd
    mov al, 0xAE
    out 0x64, al
    call .wait_kbd
    
    ; Method 3: Fast A20
    in al, 0x92
    or al, 2
    out 0x92, al
    
.done:
    ret

.wait_kbd:
    in al, 0x64
    test al, 2
    jnz .wait_kbd
    ret

.wait_data:
    in al, 0x64
    test al, 1
    jz .wait_data
    ret

; ============================================================================
; FUNCTION: verify_a20
; ============================================================================
verify_a20:
    push ds
    push es
    push di
    push si
    
    xor ax, ax
    mov ds, ax
    mov ax, 0xFFFF
    mov es, ax
    
    mov di, 0x7E00
    mov si, 0x7E10
    
    mov byte [ds:di], 0x00
    mov byte [es:si], 0xFF
    
    cmp byte [ds:di], 0xFF
    
    pop si
    pop di
    pop es
    pop ds
    
    je .failed
    clc
    ret
    
.failed:
    stc
    ret

; ============================================================================
; FUNCTION: fatal_error
; ============================================================================
fatal_error:
    push ax
    
    mov si, msg_error_prefix
    call print_string
    
    pop ax
    call print_hex_byte
    
    mov si, msg_error_suffix
    call print_string
    
    cli
.hang:
    hlt
    jmp .hang

; ============================================================================
; FUNCTION: print_hex_byte
; ============================================================================
print_hex_byte:
    push ax
    
    shr al, 4
    call .nibble
    
    pop ax
    and al, 0x0F
    call .nibble
    ret

.nibble:
    cmp al, 9
    jle .digit
    add al, 7
.digit:
    add al, '0'
    mov ah, 0x0E
    int 0x10
    ret

; ============================================================================
; FUNCTION: print_string
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
; PROTECTED MODE (32-bit)
; ============================================================================
[BITS 32]
protected_mode_start:
    ; Setup segments
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Setup stack
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Jump to kernel
    call KERNEL_OFFSET
    
    ; If kernel returns
    cli
.hang:
    hlt
    jmp .hang

; ============================================================================
; GDT (Global Descriptor Table)
; ============================================================================
[BITS 16]
gdt_start:

gdt_null:
    dq 0x0

gdt_code:
    dw 0xFFFF                   ; Limit 0-15
    dw 0x0000                   ; Base 0-15
    db 0x00                     ; Base 16-23
    db 10011010b                ; Access: present, ring 0, code, executable, readable
    db 11001111b                ; Granularity: 4KB, 32-bit, limit 16-19
    db 0x00                     ; Base 24-31

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b                ; Access: present, ring 0, data, writable
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; ============================================================================
; DATA SECTION
; ============================================================================

; Disk Address Packet for LBA
dap_size:       db 0x10
dap_reserved:   db 0
dap_sectors:    dw 0
dap_offset:     dw 0
dap_segment:    dw 0
dap_lba_low:    dd 0
dap_lba_high:   dd 0

; Variables
BOOT_DRIVE:         db 0
lba_available:      db 0
retry_count:        db 0

; Messages
msg_banner:         db 13, 10, '=== Fluid Bootloader Stage 2 ===', 13, 10, 0
msg_check_lba:      db 'Checking LBA support... ', 0
msg_lba_yes:        db 'Available', 13, 10, 0
msg_lba_no:         db 'Not available (using CHS)', 13, 10, 0
msg_enable_a20:     db 'Enabling A20 line... ', 0
msg_loading_kernel: db 'Loading kernel... ', 0
msg_verify_sig:     db 'Verifying kernel signature... ', 0
msg_enter_pm:       db 'Entering protected mode...', 13, 10, 0
msg_ok:             db 'OK', 13, 10, 0
msg_error_prefix:   db 13, 10, 'FATAL ERROR: 0x', 0
msg_error_suffix:   db 13, 10, 'System halted.', 13, 10, 0

; Pad to 4KB (8 sectors)
times (512*8)-($-$$) db 0