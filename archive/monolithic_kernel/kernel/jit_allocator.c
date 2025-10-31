/**
 * JIT Memory Allocator Implementation
 *
 * Uses free-list allocation with block coalescence to minimize fragmentation.
 * Each pool has its own memory region and free list.
 */

#include "jit_allocator.h"
#include "vga.h"

// External C functions
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);

// ============================================================================
// Internal Structures
// ============================================================================

#define BLOCK_MAGIC 0xDEADBEEF
#define MIN_BLOCK_SIZE 32

/**
 * Block header for free list allocator
 */
typedef struct block_header {
    uint32_t magic;             // Magic number for validation
    size_t size;                // Size of usable memory (excluding header)
    struct block_header* next;  // Next block in free list
    uint32_t is_free;           // 1 if free, 0 if allocated
} block_header_t;

/**
 * Memory pool structure
 */
typedef struct {
    uint8_t* base;              // Base address of pool
    size_t total_size;          // Total size of pool
    block_header_t* free_list;  // Head of free list
    jit_pool_stats_t stats;     // Pool statistics
    int initialized;            // 1 if pool is initialized
} memory_pool_t;

// ============================================================================
// Global State
// ============================================================================

static memory_pool_t g_pools[3];  // CODE, DATA, METADATA pools
static int g_allocator_initialized = 0;

// ============================================================================
// Helper Functions
// ============================================================================

static inline int is_power_of_2(size_t n) {
    return n && !(n & (n - 1));
}

static inline size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static inline int is_aligned(void* ptr, size_t alignment) {
    return ((uintptr_t)ptr % alignment) == 0;
}

// Get block header from user pointer
static inline block_header_t* get_block_header(void* ptr) {
    return (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
}

// Get user pointer from block header
static inline void* get_user_pointer(block_header_t* block) {
    return (void*)((uint8_t*)block + sizeof(block_header_t));
}

// ============================================================================
// Pool Management
// ============================================================================

static int init_pool(memory_pool_t* pool, size_t size) {
    if (!pool || size == 0) return -1;

    // Allocate memory for pool from kernel heap
    pool->base = (uint8_t*)malloc(size);
    if (!pool->base) {
        return -1;
    }

    pool->total_size = size;
    pool->initialized = 1;

    // Initialize with one large free block
    block_header_t* initial_block = (block_header_t*)pool->base;
    initial_block->magic = BLOCK_MAGIC;
    initial_block->size = size - sizeof(block_header_t);
    initial_block->next = NULL;
    initial_block->is_free = 1;

    pool->free_list = initial_block;

    // Initialize statistics
    pool->stats.total_size = size;
    pool->stats.used_size = 0;
    pool->stats.free_size = size - sizeof(block_header_t);
    pool->stats.num_allocations = 0;
    pool->stats.num_deallocations = 0;
    pool->stats.peak_usage = 0;
    pool->stats.fragmentation_bytes = 0;

    return 0;
}

static void shutdown_pool(memory_pool_t* pool) {
    if (pool && pool->initialized && pool->base) {
        free(pool->base);
        pool->base = NULL;
        pool->initialized = 0;
    }
}

// ============================================================================
// Block Allocation
// ============================================================================

static void* alloc_from_pool(memory_pool_t* pool, size_t size, size_t alignment, uint32_t flags) {
    if (!pool || !pool->initialized || size == 0) return NULL;

    // Align size to at least MIN_BLOCK_SIZE
    size = (size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : align_up(size, 8);

    block_header_t* prev = NULL;
    block_header_t* current = pool->free_list;

    // Find suitable free block (first-fit)
    while (current) {
        if (current->magic != BLOCK_MAGIC) {
            terminal_writestring("ERROR: Corrupted block header!\n");
            return NULL;
        }

        if (current->is_free && current->size >= size) {
            // Check alignment requirements
            void* user_ptr = get_user_pointer(current);
            if (alignment > 0 && !is_aligned(user_ptr, alignment)) {
                // Skip this block, alignment doesn't match
                prev = current;
                current = current->next;
                continue;
            }

            // Found suitable block - split if large enough
            if (current->size >= size + sizeof(block_header_t) + MIN_BLOCK_SIZE) {
                // Split block
                block_header_t* new_block = (block_header_t*)((uint8_t*)current + sizeof(block_header_t) + size);
                new_block->magic = BLOCK_MAGIC;
                new_block->size = current->size - size - sizeof(block_header_t);
                new_block->next = current->next;
                new_block->is_free = 1;

                current->size = size;
                current->next = new_block;
            }

            // Mark block as allocated
            current->is_free = 0;

            // Update statistics
            pool->stats.used_size += current->size + sizeof(block_header_t);
            pool->stats.free_size -= current->size + sizeof(block_header_t);
            pool->stats.num_allocations++;

            if (pool->stats.used_size > pool->stats.peak_usage) {
                pool->stats.peak_usage = pool->stats.used_size;
            }

            void* result = get_user_pointer(current);

            // Zero memory if requested
            if (flags & JIT_ALLOC_ZEROED) {
                memset(result, 0, size);
            }

            return result;
        }

        prev = current;
        current = current->next;
    }

    // No suitable block found
    return NULL;
}

static void free_to_pool(memory_pool_t* pool, void* ptr) {
    if (!pool || !pool->initialized || !ptr) return;

    block_header_t* block = get_block_header(ptr);

    // Validate block
    if (block->magic != BLOCK_MAGIC) {
        terminal_writestring("ERROR: Invalid block magic in free!\n");
        return;
    }

    if (block->is_free) {
        terminal_writestring("WARNING: Double free detected!\n");
        return;
    }

    // Mark block as free
    block->is_free = 1;

    // Update statistics
    pool->stats.used_size -= block->size + sizeof(block_header_t);
    pool->stats.free_size += block->size + sizeof(block_header_t);
    pool->stats.num_deallocations++;

    // Coalesce with adjacent free blocks
    block_header_t* current = pool->free_list;
    while (current) {
        if (current->is_free) {
            // Check if next block is adjacent and free
            block_header_t* next = (block_header_t*)((uint8_t*)current + sizeof(block_header_t) + current->size);
            if ((uint8_t*)next < pool->base + pool->total_size && next->is_free && next->magic == BLOCK_MAGIC) {
                // Coalesce
                current->size += sizeof(block_header_t) + next->size;
                current->next = next->next;
                pool->stats.fragmentation_bytes -= sizeof(block_header_t);
            }
        }
        current = current->next;
    }
}

// ============================================================================
// Public API Implementation
// ============================================================================

int jit_allocator_init(size_t code_pool_size, size_t data_pool_size, size_t metadata_pool_size) {
    if (g_allocator_initialized) {
        return 0;  // Already initialized
    }

    // Initialize all pools
    if (init_pool(&g_pools[JIT_POOL_CODE], code_pool_size) != 0) {
        return -1;
    }

    if (init_pool(&g_pools[JIT_POOL_DATA], data_pool_size) != 0) {
        shutdown_pool(&g_pools[JIT_POOL_CODE]);
        return -1;
    }

    if (init_pool(&g_pools[JIT_POOL_METADATA], metadata_pool_size) != 0) {
        shutdown_pool(&g_pools[JIT_POOL_CODE]);
        shutdown_pool(&g_pools[JIT_POOL_DATA]);
        return -1;
    }

    g_allocator_initialized = 1;
    return 0;
}

void jit_allocator_shutdown(void) {
    if (!g_allocator_initialized) return;

    shutdown_pool(&g_pools[JIT_POOL_CODE]);
    shutdown_pool(&g_pools[JIT_POOL_DATA]);
    shutdown_pool(&g_pools[JIT_POOL_METADATA]);

    g_allocator_initialized = 0;
}

void* jit_alloc(size_t size, jit_pool_type_t pool, uint32_t flags) {
    if (!g_allocator_initialized || pool < 0 || pool > JIT_POOL_METADATA) {
        return NULL;
    }

    return alloc_from_pool(&g_pools[pool], size, 0, flags);
}

void* jit_alloc_aligned(size_t size, size_t alignment, jit_pool_type_t pool, uint32_t flags) {
    if (!g_allocator_initialized || pool < 0 || pool > JIT_POOL_METADATA) {
        return NULL;
    }

    if (!is_power_of_2(alignment)) {
        return NULL;
    }

    return alloc_from_pool(&g_pools[pool], size, alignment, flags);
}

void jit_free(void* ptr, jit_pool_type_t pool) {
    if (!g_allocator_initialized || !ptr || pool < 0 || pool > JIT_POOL_METADATA) {
        return;
    }

    free_to_pool(&g_pools[pool], ptr);
}

void* jit_realloc(void* ptr, size_t new_size, jit_pool_type_t pool, uint32_t flags) {
    if (!g_allocator_initialized || pool < 0 || pool > JIT_POOL_METADATA) {
        return NULL;
    }

    if (!ptr) {
        return jit_alloc(new_size, pool, flags);
    }

    if (new_size == 0) {
        jit_free(ptr, pool);
        return NULL;
    }

    // Get current block size
    block_header_t* block = get_block_header(ptr);
    if (block->magic != BLOCK_MAGIC) {
        return NULL;
    }

    // If new size fits in current block, just return ptr
    if (new_size <= block->size) {
        return ptr;
    }

    // Allocate new block and copy data
    void* new_ptr = jit_alloc(new_size, pool, flags);
    if (!new_ptr) {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size);
    jit_free(ptr, pool);

    return new_ptr;
}

void jit_get_pool_stats(jit_pool_type_t pool, jit_pool_stats_t* stats) {
    if (!g_allocator_initialized || !stats || pool < 0 || pool > JIT_POOL_METADATA) {
        return;
    }

    *stats = g_pools[pool].stats;
}

void jit_print_pool_stats(int pool) {
    if (!g_allocator_initialized) {
        terminal_writestring("JIT allocator not initialized\n");
        return;
    }

    const char* pool_names[] = {"CODE", "DATA", "METADATA"};

    int start = (pool == -1) ? 0 : pool;
    int end = (pool == -1) ? 2 : pool;

    for (int i = start; i <= end; i++) {
        terminal_writestring("\n--- JIT Pool ");
        terminal_writestring(pool_names[i]);
        terminal_writestring(" ---\n");

        jit_pool_stats_t stats = g_pools[i].stats;

        terminal_writestring("Total: ");
        // Print numbers (simplified, you'd want proper formatting)
        terminal_writestring(" bytes\n");

        terminal_writestring("Used: ");
        terminal_writestring(" bytes\n");

        terminal_writestring("Free: ");
        terminal_writestring(" bytes\n");
    }
}

size_t jit_defragment_pool(jit_pool_type_t pool) {
    // TODO: Implement defragmentation if needed
    // For now, coalescing happens automatically during free()
    (void)pool;
    return 0;
}

void jit_reset_pool(jit_pool_type_t pool) {
    if (!g_allocator_initialized || pool < 0 || pool > JIT_POOL_METADATA) {
        return;
    }

    memory_pool_t* p = &g_pools[pool];

    // Re-initialize pool with one large free block
    block_header_t* initial_block = (block_header_t*)p->base;
    initial_block->magic = BLOCK_MAGIC;
    initial_block->size = p->total_size - sizeof(block_header_t);
    initial_block->next = NULL;
    initial_block->is_free = 1;

    p->free_list = initial_block;

    // Reset statistics
    p->stats.used_size = 0;
    p->stats.free_size = p->total_size - sizeof(block_header_t);
    p->stats.num_allocations = 0;
    p->stats.num_deallocations = 0;
}

int jit_is_pool_pointer(const void* ptr, jit_pool_type_t* pool) {
    if (!g_allocator_initialized || !ptr) {
        return 0;
    }

    for (int i = 0; i <= JIT_POOL_METADATA; i++) {
        memory_pool_t* p = &g_pools[i];
        if (p->initialized &&
            (uint8_t*)ptr >= p->base &&
            (uint8_t*)ptr < p->base + p->total_size) {
            if (pool) *pool = (jit_pool_type_t)i;
            return 1;
        }
    }

    return 0;
}

int jit_mark_executable(void* ptr, size_t size) {
    // In bare-metal, all memory is executable by default
    // This function is a no-op but provided for API compatibility
    (void)ptr;
    (void)size;
    return 0;
}
