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
    #define HEAP_SIZE (64 * 1024 * 1024)    // 64 MB for bare-metal
#else
    #define HEAP_SIZE (200 * 1024 * 1024)   // 200 MB for userspace
#endif

// ============================================================================
// Heap Storage
// ============================================================================

// SOLUTION: Use a static array in .bss section
// This ensures the heap is in the same memory region as the kernel
// and properly mapped by the bootloader - no manual address needed!
static unsigned char heap[HEAP_SIZE] __attribute__((aligned(16)));
static unsigned long heap_offset = 0;

// ============================================================================
// malloc() - Bump Allocator
// ============================================================================

// External serial for debugging
extern void serial_puts(const char* str);
extern void serial_put_uint64(unsigned long value);

void* malloc(unsigned long size) {
    if (size == 0)
        return NULL;

    // Align size to 16 bytes
    size = (size + 15) & ~15;

    // Check if we have enough space
    if (heap_offset + size > HEAP_SIZE) {
        return NULL;  // Out of memory
    }

    // Allocate by bumping offset
    void* ptr = &heap[heap_offset];
    heap_offset += size;

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
    return heap_offset;
}

unsigned long malloc_get_peak(void) {
    return heap_offset;  // Same as usage for bump allocator
}

unsigned long malloc_get_heap_size(void) {
    return HEAP_SIZE;
}
