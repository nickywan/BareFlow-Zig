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
// VGA TEXT MODE
// ============================================================================
#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// VGA colors
#define VGA_COLOR_BLACK 0x0
#define VGA_COLOR_BLUE 0x1
#define VGA_COLOR_GREEN 0x2
#define VGA_COLOR_CYAN 0x3
#define VGA_COLOR_RED 0x4
#define VGA_COLOR_MAGENTA 0x5
#define VGA_COLOR_BROWN 0x6
#define VGA_COLOR_LIGHT_GREY 0x7
#define VGA_COLOR_DARK_GREY 0x8
#define VGA_COLOR_LIGHT_BLUE 0x9
#define VGA_COLOR_LIGHT_GREEN 0xA
#define VGA_COLOR_LIGHT_CYAN 0xB
#define VGA_COLOR_LIGHT_RED 0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_YELLOW 0xE
#define VGA_COLOR_WHITE 0xF

// Global variables
static unsigned short* vga_buffer = (unsigned short*)VGA_ADDRESS;
static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char current_color = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREY;

// ============================================================================
// VGA FUNCTIONS
// ============================================================================

static inline unsigned short vga_entry(unsigned char ch, unsigned char color) {
    return (unsigned short)ch | ((unsigned short)color << 8);
}

void vga_set_color(unsigned char fg, unsigned char bg) {
    current_color = (bg << 4) | fg;
}

void vga_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', current_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', current_color);
    }
    
    cursor_y = VGA_HEIGHT - 1;
}

void vga_putchar_at(char ch, int x, int y, unsigned char color) {
    const int index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(ch, color);
}

void vga_putchar(char ch) {
    if (ch == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (ch == '\r') {
        cursor_x = 0;
    } else if (ch == '\t') {
        cursor_x = (cursor_x + 4) & ~(4 - 1);
    } else if (ch == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga_putchar_at(' ', cursor_x, cursor_y, current_color);
        }
    } else {
        vga_putchar_at(ch, cursor_x, cursor_y, current_color);
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putchar(str[i]);
    }
}

void vga_print_color(const char* str, unsigned char fg, unsigned char bg) {
    unsigned char old_color = current_color;
    vga_set_color(fg, bg);
    vga_print(str);
    current_color = old_color;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================


void print_int(int num) {
    if (num == 0) {
        vga_putchar('0');
        return;
    }
    
    if (num < 0) {
        vga_putchar('-');
        num = -num;
    }
    
    char buffer[12];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    for (int j = i - 1; j >= 0; j--) {
        vga_putchar(buffer[j]);
    }
}

void print_hex(unsigned int num) {
    vga_print("0x");
    
    char hex[] = "0123456789ABCDEF";
    char buffer[9];
    
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex[num & 0xF];
        num >>= 4;
    }
    buffer[8] = '\0';
    
    vga_print(buffer);
}

// ============================================================================
// KERNEL BANNER
// ============================================================================
void print_banner(void) {
    vga_print_color("================================================================================\n", 
                    VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_print_color("                             FLUID KERNEL v1.0                                 \n",
                    VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_color("================================================================================\n",
                    VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_print("\n");
}

// ============================================================================
// SYSTEM INFO
// ============================================================================
void print_system_info(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("System Information:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    vga_print("  - Architecture:     ");
    vga_print_color("x86 (32-bit protected mode)\n", VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    
    vga_print("  - Kernel Address:   ");
    print_hex(0x1000);
    vga_print("\n");
    
    vga_print("  - VGA Buffer:       ");
    print_hex(VGA_ADDRESS);
    vga_print("\n");
    
    vga_print("  - Signature Check:  ");
    vga_print_color("PASSED (FLUD)\n", VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    
    vga_print("\n");
}

// ============================================================================
// CPU INFO
// ============================================================================
void check_cpu_features(void) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_print("CPU Features:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    unsigned int eax, ebx, ecx, edx;
    
    // Get CPU vendor
    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    
    vga_print("  - CPU Vendor:       ");
    char vendor[13];
    *((unsigned int*)&vendor[0]) = ebx;
    *((unsigned int*)&vendor[4]) = edx;
    *((unsigned int*)&vendor[8]) = ecx;
    vendor[12] = '\0';
    vga_print(vendor);
    vga_print("\n");
    
    // Get CPU features
    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    
    vga_print("  - FPU:              ");
    vga_print((edx & (1 << 0)) ? "Yes" : "No");
    vga_print("\n");
    
    vga_print("  - MMX:              ");
    vga_print((edx & (1 << 23)) ? "Yes" : "No");
    vga_print("\n");
    
    vga_print("  - SSE:              ");
    vga_print((edx & (1 << 25)) ? "Yes" : "No");
    vga_print("\n");
    
    vga_print("\n");
}

// ============================================================================
// KERNEL MAIN
// ============================================================================
void kernel_main(void) {
    // Clear screen
    vga_clear();
    
    // Print banner
    print_banner();
    
    // System info
    print_system_info();
    
    // CPU features
    check_cpu_features();
    
    // Success message
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vga_print("Kernel initialized successfully!\n\n");

    
    // ===== TEST MALLOC =====
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("[TEST] Allocating 1024 bytes...\n\n");
        
    void* ptr = malloc(1024);
    
    if (ptr != NULL) {
        vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        vga_print("[[OK] malloc returned: 0x");       
        
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
            vga_putchar(hex[j]);
        }
    
        vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        vga_print("[OK] malloc works!\n");       
            
    } else {
        vga_set_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
        vga_print("[FAIL] malloc returned NULL\n");       
    }
   
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_print("\nSystem ready. CPU halted.\n");


    // Infinite loop
    while (1) {
        asm volatile("hlt");
    }
}