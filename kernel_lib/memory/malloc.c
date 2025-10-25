/**
 * Memory Allocator - Implementation
 *
 * Simple bump allocator: allocates linearly from a fixed heap.
 * No real free() - memory is reclaimed only on reboot.
 */

#include "malloc.h"
#include "string.h"
#include <stdint.h>

// ============================================================================
// HEAP CONFIGURATION
// ============================================================================

#define HEAP_SIZE (256 * 1024)  // 256 KB heap
static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static size_t heap_offset = 0;

// ============================================================================
// ALLOCATOR IMPLEMENTATION
// ============================================================================

void* malloc(size_t size) {
    if (size == 0)
        return NULL;

    // Align to 16 bytes
    size = (size + 15) & ~15;

    if (heap_offset + size > HEAP_SIZE)
        return NULL;

    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr)
        return malloc(size);

    void* new_ptr = malloc(size);
    if (new_ptr && ptr) {
        memcpy(new_ptr, ptr, size);
    }
    return new_ptr;
}

void free(void* ptr) {
    // Bump allocator: no-op
    (void)ptr;
}
