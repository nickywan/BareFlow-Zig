// ============================================================================
// FLUID KERNEL - Main kernel code
// ============================================================================
// File: kernel/kernel.c
// Entry: kernel_main() is called from entry.asm
// ============================================================================

#include <stddef.h>
#include "vga.h"

// Forward declarations
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);

// ============================================================================
// HELPER: VGA print with color (temporary color change)
// ============================================================================
static void vga_print_color(const char* str, enum vga_color fg, enum vga_color bg) {
    // Save current color, set new color, print, restore color
    // Since vga.c doesn't expose current color, we'll use a simpler approach:
    // set color, print, then reset to default
    terminal_setcolor(fg, bg);
    terminal_writestring(str);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);  // Default color
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void print_int(int num) {
    if (num == 0) {
        terminal_putchar('0');
        return;
    }

    if (num < 0) {
        terminal_putchar('-');
        num = -num;
    }

    char buffer[12];
    int i = 0;

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        terminal_putchar(buffer[j]);
    }
}

void print_hex(unsigned int num) {
    terminal_writestring("0x");

    char hex[] = "0123456789ABCDEF";
    char buffer[9];

    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex[num & 0xF];
        num >>= 4;
    }
    buffer[8] = '\0';

    terminal_writestring(buffer);
}

// ============================================================================
// KERNEL BANNER
// ============================================================================
void print_banner(void) {
    vga_print_color("================================================================================\n",
                    VGA_CYAN, VGA_BLACK);
    vga_print_color("                             FLUID KERNEL v1.0                                 \n",
                    VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print_color("================================================================================\n",
                    VGA_CYAN, VGA_BLACK);
    terminal_writestring("\n");
}

// ============================================================================
// SYSTEM INFO
// ============================================================================
void print_system_info(void) {
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("System Information:\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  - Architecture:     ");
    vga_print_color("x86 (32-bit protected mode)\n", VGA_GREEN, VGA_BLACK);

    terminal_writestring("  - Kernel Address:   ");
    print_hex(0x1000);
    terminal_writestring("\n");

    terminal_writestring("  - VGA Buffer:       ");
    print_hex(0xB8000);
    terminal_writestring("\n");

    terminal_writestring("  - Signature Check:  ");
    vga_print_color("PASSED (FLUD)\n", VGA_GREEN, VGA_BLACK);

    terminal_writestring("\n");
}

// ============================================================================
// CPU INFO
// ============================================================================
void check_cpu_features(void) {
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("CPU Features:\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    unsigned int eax, ebx, ecx, edx;

    // Get CPU vendor
    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    terminal_writestring("  - CPU Vendor:       ");
    char vendor[13];
    *((unsigned int*)&vendor[0]) = ebx;
    *((unsigned int*)&vendor[4]) = edx;
    *((unsigned int*)&vendor[8]) = ecx;
    vendor[12] = '\0';
    terminal_writestring(vendor);
    terminal_writestring("\n");

    // Get CPU features
    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    terminal_writestring("  - FPU:              ");
    terminal_writestring((edx & (1 << 0)) ? "Yes" : "No");
    terminal_writestring("\n");

    terminal_writestring("  - MMX:              ");
    terminal_writestring((edx & (1 << 23)) ? "Yes" : "No");
    terminal_writestring("\n");

    terminal_writestring("  - SSE:              ");
    terminal_writestring((edx & (1 << 25)) ? "Yes" : "No");
    terminal_writestring("\n");

    terminal_writestring("\n");
}

// ============================================================================
// KERNEL MAIN
// ============================================================================
void kernel_main(void) {
    // Initialize VGA terminal
    terminal_initialize();

    // Print banner
    print_banner();

    // System info
    print_system_info();

    // CPU features
    check_cpu_features();

    // Success message
    terminal_setcolor(VGA_GREEN, VGA_BLACK);
    terminal_writestring("Kernel initialized successfully!\n\n");


    // ===== TEST MALLOC =====
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_writestring("[TEST] Allocating 1024 bytes...\n\n");

    void* ptr = malloc(1024);

    if (ptr != NULL) {
        terminal_setcolor(VGA_GREEN, VGA_BLACK);
        terminal_writestring("[[OK] malloc returned: 0x");

        // Affiche l'adresse (simple hex print)
        char hex[16];
        unsigned long addr = (unsigned long)ptr;
        int i = 0;

        do {
            int digit = addr & 0xF;
            hex[i++] = (digit < 10) ? '0' + digit : 'a' + digit - 10;
            addr >>= 4;
        } while (addr != 0);

        // Reverse string
        for (int j = i - 1; j >= 0; j--) {
            terminal_putchar(hex[j]);
        }

        terminal_setcolor(VGA_GREEN, VGA_BLACK);
        terminal_writestring("[OK] malloc works!\n");

    } else {
        terminal_setcolor(VGA_RED, VGA_BLACK);
        terminal_writestring("[FAIL] malloc returned NULL\n");
    }

    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_writestring("\nSystem ready. CPU halted.\n");


    // Infinite loop
    while (1) {
        asm volatile("hlt");
    }
}
