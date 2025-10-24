#ifndef CXX_RUNTIME_H
#define CXX_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// C++ Runtime Support for Bare-Metal
// ============================================================================

/**
 * Initialize C++ runtime
 * - Sets up memory allocator for C++ objects
 * - Calls global constructors
 */
void cxx_runtime_init(void);

/**
 * Cleanup C++ runtime
 * - Calls global destructors
 */
void cxx_runtime_fini(void);

/**
 * Get allocation statistics
 */
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_used;
    size_t num_allocations;
    size_t num_deallocations;
} cxx_alloc_stats_t;

void cxx_get_alloc_stats(cxx_alloc_stats_t* stats);

/**
 * Set custom allocator for C++ objects
 * If not set, uses kernel malloc/free
 */
typedef void* (*cxx_alloc_fn)(size_t size);
typedef void (*cxx_free_fn)(void* ptr);

void cxx_set_allocator(cxx_alloc_fn alloc, cxx_free_fn free);

#ifdef __cplusplus
}

// ============================================================================
// C++ Operator Overloads (must be in global namespace)
// ============================================================================

// Single object allocation
void* operator new(size_t size);
void* operator new(size_t size, void* ptr) noexcept;  // Placement new
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;

// Array allocation
void* operator new[](size_t size);
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;

#endif // __cplusplus

#endif // CXX_RUNTIME_H
