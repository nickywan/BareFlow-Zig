/**
 * Bump Allocator - Simplest Possible malloc() for Investigation
 *
 * This is a VERY simple bump allocator for debugging:
 * - malloc() just bumps a pointer forward
 * - free() does nothing
 * - No fragmentation handling
 * - No free-list complexity
 *
 * Purpose: Isolate whether the malloc issue is in the free-list code
 */

#include <stddef.h>

// ============================================================================
// Configuration
// ============================================================================

#ifdef HEAP_SIZE_SMALL
    #define HEAP_SIZE (256 * 1024)          // 256 KB for QEMU kernel
#elif defined(BARE_METAL)
    #define HEAP_SIZE (32 * 1024 * 1024)    // 32 MB for bare-metal (safe for .bss)
#else
    #define HEAP_SIZE (200 * 1024 * 1024)   // 200 MB for userspace
#endif

// ============================================================================
// Heap Storage
// ============================================================================

// SOLUTION: Use static variables but initialize them at RUNTIME (not compile-time)
#define HEAP_START_ADDR     0x2100000   // 33 MB - heap data start
#define HEAP_INIT_MAGIC     0xDEADBEEF

// Static variables - will be in .bss (garbage at boot), initialized on first malloc()
static unsigned long heap_init_flag;
static unsigned char* heap_ptr;
static unsigned char* heap_end;

// ============================================================================
// malloc() - Bump Allocator
// ============================================================================

// External serial for debugging
extern void serial_puts(const char* str);
extern void serial_put_uint64(unsigned long value);

void* malloc(unsigned long size) {
    // Initialize heap on first call (check if flag is NOT the magic value)
    if (heap_init_flag != HEAP_INIT_MAGIC) {
        heap_ptr = (unsigned char*)HEAP_START_ADDR;
        heap_end = (unsigned char*)(HEAP_START_ADDR + HEAP_SIZE);
        heap_init_flag = HEAP_INIT_MAGIC;
    }

    // Align size to 16 bytes
    size = (size + 15) & ~15;

    // Check if we have enough space
    if (heap_ptr + size > heap_end) {
        return NULL;
    }

    // Allocate by bumping pointer
    void* ptr = heap_ptr;
    heap_ptr += size;

    return ptr;
}

// ============================================================================
// free() - No-op for Bump Allocator
// ============================================================================

void free(void* ptr) {
    // Bump allocator doesn't free - this is intentional
    (void)ptr;  // Suppress unused warning
}

// ============================================================================
// calloc() - Zero-initialized allocation
// ============================================================================

void* calloc(unsigned long nmemb, unsigned long size) {
    unsigned long total = nmemb * size;
    void* ptr = malloc(total);

    if (ptr) {
        // Zero out memory
        unsigned char* p = (unsigned char*)ptr;
        for (unsigned long i = 0; i < total; i++) {
            p[i] = 0;
        }
    }

    return ptr;
}

// ============================================================================
// realloc() - Simple implementation
// ============================================================================

void* realloc(void* ptr, unsigned long size) {
    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // For bump allocator, we always allocate new memory
    // (can't determine old size without metadata)
    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    // Note: This is NOT a correct realloc (we don't know old size)
    // But for testing purposes, we copy a reasonable amount
    unsigned char* src = (unsigned char*)ptr;
    unsigned char* dst = (unsigned char*)new_ptr;
    for (unsigned long i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    return new_ptr;
}

// ============================================================================
// Query Functions
// ============================================================================

unsigned long malloc_get_usage(void) {
    if (heap_init_flag != HEAP_INIT_MAGIC) return 0;
    return (unsigned long)(heap_ptr - (unsigned char*)HEAP_START_ADDR);
}

unsigned long malloc_get_peak(void) {
    return malloc_get_usage();  // Same as usage for bump allocator
}

unsigned long malloc_get_heap_size(void) {
    return HEAP_SIZE;
}
