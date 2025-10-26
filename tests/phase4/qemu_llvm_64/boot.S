/**
 * Multiboot2 Boot Entry for 64-bit Kernel with LLVM
 */

.section .multiboot
.align 8

multiboot_header_start:
    .long 0xE85250D6                    // Multiboot2 magic
    .long 0                             // Architecture: i386
    .long multiboot_header_end - multiboot_header_start  // Header length
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))  // Checksum

    // End tag
    .short 0                            // type
    .short 0                            // flags
    .long 8                             // size
multiboot_header_end:

.section .bss

// Page Tables (4-level paging for x86-64, using 2 MB pages)
.align 4096
pml4_table:
    .skip 4096                          // PML4: 512 entries × 8 bytes
pdpt_table:
    .skip 4096                          // PDPT: 512 entries × 8 bytes
pd_table:
    .skip 4096                          // PD: 512 entries × 8 bytes (2 MB pages)

// Stack
.align 16
stack_bottom:
    .skip 16384                         // 16 KB stack
stack_top:

.section .text
.global _start
.type _start, @function

// ============================================================================
// init_paging: Setup 4-level page tables with 2 MB pages
// ============================================================================
init_paging:
    // Page table entry flags:
    // Bit 0: Present (P)
    // Bit 1: Writable (R/W)
    // Bit 7: Page Size (PS) - 2 MB pages
    // FLAGS = 0x83 (Present + Writable + Page Size)

    // 1. Setup PML4[0] → PDPT
    movabs $pml4_table, %rdi
    movabs $pdpt_table, %rax
    or $0x003, %rax             // Present + Writable
    mov %rax, (%rdi)            // PML4[0] = pdpt_table | 0x003

    // 2. Setup PDPT[0] → PD (first 1 GB)
    movabs $pdpt_table, %rdi
    movabs $pd_table, %rax
    or $0x003, %rax             // Present + Writable
    mov %rax, (%rdi)            // PDPT[0] = pd_table | 0x003

    // 3. Setup PD entries using 2 MB pages (identity map 0-512 MB)
    // Each PD entry maps 2 MB with PS bit set
    // 512 MB / 2 MB = 256 entries

    movabs $pd_table, %rdi
    mov $256, %rcx              // Map 256 × 2 MB = 512 MB
    xor %rax, %rax              // Physical address starts at 0

.fill_pd_loop:
    mov %rax, %rdx
    or $0x083, %rdx             // Present + Writable + Page Size (2 MB)
    mov %rdx, (%rdi)            // PD[i] = physical_addr | 0x083
    add $(2 * 1024 * 1024), %rax // Next 2 MB
    add $8, %rdi                // Next PD entry
    loop .fill_pd_loop

    // 4. Load CR3 with PML4 address
    movabs $pml4_table, %rax
    mov %rax, %cr3              // Load new page tables

    ret

// ============================================================================
// _start: Entry point
// ============================================================================
_start:
    // Setup stack
    mov $stack_top, %rsp

    // Clear direction flag
    cld

    // Zero .bss section (CRITICAL for malloc)
    movabs $_bss_start, %rdi
    movabs $_bss_end, %rcx
    sub %rdi, %rcx          // size = end - start
    xor %eax, %eax          // value = 0
    rep stosb               // memset(bss, 0, size)

    // Initialize paging (CRITICAL for malloc global variables)
    call init_paging

    // Call kernel main
    call kernel_main

    // Halt if kernel returns
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
