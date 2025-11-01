/**
 * Simple malloc with static heap in .bss
 * This overrides the one from kernel_lib_llvm.a
 */

#include <stddef.h>
#include <stdint.h>

#define HEAP_SIZE (64 * 1024 * 1024)  // 64 MB
static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static size_t heap_offset = 0;

void* malloc(size_t size) {
    if (size == 0) return NULL;
    
    // Align to 16 bytes
    size = (size + 15) & ~15;
    
    if (heap_offset + size > HEAP_SIZE)
        return NULL;
    
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void free(void* ptr) {
    (void)ptr;  // No-op for bump allocator
}

size_t malloc_get_usage(void) {
    return heap_offset;
}

size_t malloc_get_peak(void) {
    return heap_offset;  // Same as usage for bump allocator
}

size_t malloc_get_heap_size(void) {
    return HEAP_SIZE;
}
