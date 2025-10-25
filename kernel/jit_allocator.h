#ifndef JIT_ALLOCATOR_H
#define JIT_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// JIT Memory Allocator
// ============================================================================
// Specialized allocator for JIT-compiled code and LLVM runtime structures
//
// Features:
// - Separate pools for code (executable) and data
// - Fast allocation/deallocation for small objects
// - Memory statistics and debugging
// - Alignment support for code requirements

/**
 * Memory pool types
 */
typedef enum {
    JIT_POOL_CODE,      // Executable code (needs special permissions in protected OS)
    JIT_POOL_DATA,      // JIT runtime data structures
    JIT_POOL_METADATA,  // Small metadata objects
} jit_pool_type_t;

/**
 * Allocation flags
 */
#define JIT_ALLOC_EXECUTABLE  0x01  // Memory should be executable
#define JIT_ALLOC_ZEROED      0x02  // Zero memory after allocation
#define JIT_ALLOC_ALIGNED     0x04  // Align to specific boundary

/**
 * Memory statistics
 */
typedef struct {
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t num_allocations;
    size_t num_deallocations;
    size_t peak_usage;
    size_t fragmentation_bytes;
} jit_pool_stats_t;

/**
 * Initialize JIT allocator
 *
 * @param code_pool_size Size of executable code pool (bytes)
 * @param data_pool_size Size of data pool (bytes)
 * @param metadata_pool_size Size of metadata pool (bytes)
 * @return 0 on success, -1 on failure
 */
int jit_allocator_init(size_t code_pool_size, size_t data_pool_size, size_t metadata_pool_size);

/**
 * Shutdown JIT allocator and free all memory
 */
void jit_allocator_shutdown(void);

/**
 * Allocate memory from JIT pool
 *
 * @param size Size in bytes
 * @param pool Pool to allocate from
 * @param flags Allocation flags (JIT_ALLOC_*)
 * @return Pointer to allocated memory, or NULL on failure
 */
void* jit_alloc(size_t size, jit_pool_type_t pool, uint32_t flags);

/**
 * Allocate aligned memory from JIT pool
 *
 * @param size Size in bytes
 * @param alignment Alignment in bytes (must be power of 2)
 * @param pool Pool to allocate from
 * @param flags Allocation flags
 * @return Pointer to aligned memory, or NULL on failure
 */
void* jit_alloc_aligned(size_t size, size_t alignment, jit_pool_type_t pool, uint32_t flags);

/**
 * Free memory allocated by jit_alloc
 *
 * @param ptr Pointer to memory
 * @param pool Pool that allocated the memory
 */
void jit_free(void* ptr, jit_pool_type_t pool);

/**
 * Reallocate memory
 *
 * @param ptr Original pointer (can be NULL)
 * @param new_size New size in bytes
 * @param pool Pool to allocate from
 * @param flags Allocation flags
 * @return Pointer to reallocated memory, or NULL on failure
 */
void* jit_realloc(void* ptr, size_t new_size, jit_pool_type_t pool, uint32_t flags);

/**
 * Get pool statistics
 *
 * @param pool Pool to query
 * @param stats Output statistics structure
 */
void jit_get_pool_stats(jit_pool_type_t pool, jit_pool_stats_t* stats);

/**
 * Print pool statistics (for debugging)
 *
 * @param pool Pool to print, or -1 for all pools
 */
void jit_print_pool_stats(int pool);

/**
 * Defragment a pool (compact free blocks)
 *
 * @param pool Pool to defragment
 * @return Number of bytes reclaimed
 */
size_t jit_defragment_pool(jit_pool_type_t pool);

/**
 * Reset a pool (free all allocations)
 * WARNING: This invalidates all pointers from this pool!
 *
 * @param pool Pool to reset
 */
void jit_reset_pool(jit_pool_type_t pool);

/**
 * Check if pointer belongs to a JIT pool
 *
 * @param ptr Pointer to check
 * @param pool Output pool type (if pointer belongs to a pool)
 * @return 1 if pointer is from a JIT pool, 0 otherwise
 */
int jit_is_pool_pointer(const void* ptr, jit_pool_type_t* pool);

/**
 * Mark memory as executable (no-op on bare-metal, but provided for compatibility)
 *
 * @param ptr Pointer to memory
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int jit_mark_executable(void* ptr, size_t size);

/**
 * Allocate executable code memory (convenience wrapper for Micro-JIT)
 *
 * @param size Size in bytes
 * @return Pointer to executable memory, or NULL on failure
 */
static inline void* jit_alloc_code(size_t size) {
    return jit_alloc(size, JIT_POOL_CODE, JIT_ALLOC_EXECUTABLE | JIT_ALLOC_ZEROED);
}

/**
 * Free executable code memory (convenience wrapper for Micro-JIT)
 *
 * @param ptr Pointer to memory
 */
static inline void jit_free_code(void* ptr) {
    if (ptr) {
        jit_free(ptr, JIT_POOL_CODE);
    }
}

#ifdef __cplusplus
}
#endif

#endif // JIT_ALLOCATOR_H
