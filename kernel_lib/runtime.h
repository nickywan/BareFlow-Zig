/**
 * Runtime Library - Public API
 *
 * Unified header for I/O, Memory, and CPU functions.
 * This is the main header to include in applications.
 */

#ifndef RUNTIME_H
#define RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include "cpu/features.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// I/O - VGA Text Mode
// ============================================================================

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_writestring(const char* str);
void terminal_setcolor(uint8_t fg, uint8_t bg);
void terminal_write(const char* data, size_t size);

// Aliases for convenience
#define vga_init terminal_initialize
#define vga_putchar terminal_putchar
#define vga_writestring terminal_writestring
#define vga_setcolor terminal_setcolor
#define vga_write terminal_write

// ============================================================================
// I/O - Serial Port (COM1)
// ============================================================================

int serial_init(void);
void serial_putchar(char c);
void serial_puts(const char* str);
void serial_put_int(int value);
void serial_put_uint(unsigned int value);
void serial_put_uint64(uint64_t value);

// ============================================================================
// I/O - Keyboard (PS/2)
// ============================================================================

int keyboard_has_key(void);
uint8_t keyboard_read(void);
void wait_key(void);

// ============================================================================
// MEMORY - Allocator
// ============================================================================

void* malloc(size_t size);
void* calloc(size_t nmemb, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

// ============================================================================
// MEMORY - String Functions
// ============================================================================

void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcat(char* dest, const char* src);

// ============================================================================
// CPU - Features and Utilities
// ============================================================================
// Note: cpu_rdtsc() is provided by cpu/features.h as static inline

void cpu_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);
int cpu_has_sse(void);
int cpu_has_sse2(void);
int cpu_has_avx(void);

// ============================================================================
// CPU - Interrupts (PIC/IDT)
// ============================================================================

void pic_init(void);
void idt_init(void);

#ifdef __cplusplus
}
#endif

#endif // RUNTIME_H
