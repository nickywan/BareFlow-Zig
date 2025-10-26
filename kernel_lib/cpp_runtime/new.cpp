/**
 * C++ Runtime - operator new/delete
 *
 * Bare-metal implementation using kernel_lib malloc/free.
 * Required for any C++ code that uses heap allocation.
 */

#include <stddef.h>

// Forward declarations from kernel_lib/memory/malloc.h
extern "C" {
    void* malloc(size_t size);
    void* calloc(size_t nmemb, size_t size);
    void free(void* ptr);
}

// ============================================================================
// operator new
// ============================================================================

void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

// Placement new (no-op, just returns the pointer)
void* operator new(size_t, void* ptr) {
    return ptr;
}

void* operator new[](size_t, void* ptr) {
    return ptr;
}

// ============================================================================
// operator delete
// ============================================================================

void operator delete(void* ptr) noexcept {
    free(ptr);
}

void operator delete[](void* ptr) noexcept {
    free(ptr);
}

// Sized delete (C++14)
void operator delete(void* ptr, size_t) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    free(ptr);
}

// Placement delete (no-op)
void operator delete(void*, void*) noexcept {
    // No-op for placement delete
}

void operator delete[](void*, void*) noexcept {
    // No-op for placement delete
}
