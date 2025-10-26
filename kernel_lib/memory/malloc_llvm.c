/**
 * LLVM Memory Allocator - Free-list Implementation
 *
 * Enhanced allocator for LLVM integration:
 * - Large heap (200 MB) for LLVM runtime
 * - Proper free() with coalescing
 * - Large block support (up to 10 MB)
 * - Free-list based allocation
 *
 * Strategy: Segregated free lists with first-fit allocation
 */

// For bare-metal: use local definitions
#ifndef __STDBOOL_H
typedef _Bool bool;
#define true 1
#define false 0
#endif

#ifndef __STDINT_H
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long size_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// Forward declarations for string functions
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

// Debug serial output (if available)
#ifdef DEBUG_MALLOC
extern void serial_putchar(char c);
static void debug_print(const char* msg) {
    while (*msg) {
        serial_putchar(*msg++);
    }
}
#else
#define debug_print(msg) ((void)0)
#endif

// ============================================================================
// CONFIGURATION
// ============================================================================

// Heap size configuration
// - QEMU kernel: 256 KB (in .data section, must be small)
// - Userspace test: 32 MB
// - Full bare-metal with paging: 200 MB (future)
#ifdef HEAP_SIZE_SMALL
#define HEAP_SIZE (256 * 1024)         // 256 KB for QEMU kernel (.data section)
#elif defined(BARE_METAL)
#define HEAP_SIZE (32 * 1024 * 1024)   // 32 MB for bare-metal (safe for .bss)
#else
#define HEAP_SIZE (32 * 1024 * 1024)   // 32 MB for userspace testing
#endif

#define MIN_BLOCK_SIZE 32              // Minimum allocation size
#define ALIGNMENT 16                   // 16-byte alignment

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Block header for free-list
typedef struct Block {
    size_t size;            // Size of this block (including header)
    bool is_free;           // Free or allocated
    struct Block* next;     // Next block in free list
    struct Block* prev;     // Previous block in free list
} Block;

#define BLOCK_HEADER_SIZE sizeof(Block)

// ============================================================================
// HEAP STORAGE
// ============================================================================

// Heap in .bss (uninitialized)
static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static Block* free_list = NULL;
static bool heap_initialized = false;

// ============================================================================
// STATISTICS (for debugging)
// ============================================================================

static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t current_usage = 0;
static size_t peak_usage = 0;
static size_t num_allocations = 0;
static size_t num_frees = 0;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Align size to ALIGNMENT boundary
static inline size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Get pointer to user data from block
static inline void* block_to_ptr(Block* block) {
    return (void*)((uint8_t*)block + BLOCK_HEADER_SIZE);
}

// Get block from user pointer
static inline Block* ptr_to_block(void* ptr) {
    return (Block*)((uint8_t*)ptr - BLOCK_HEADER_SIZE);
}

// Check if pointer is within heap bounds
static inline bool is_valid_heap_ptr(void* ptr) {
    return ptr >= (void*)heap && ptr < (void*)(heap + HEAP_SIZE);
}

// ============================================================================
// FREE LIST MANAGEMENT
// ============================================================================

// Remove block from free list
static void remove_from_free_list(Block* block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_list = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }

    block->next = NULL;
    block->prev = NULL;
}

// Add block to free list (sorted by address for coalescing)
static void add_to_free_list(Block* block) {
    block->is_free = true;

    if (!free_list || block < free_list) {
        // Insert at head
        block->next = free_list;
        block->prev = NULL;
        if (free_list) {
            free_list->prev = block;
        }
        free_list = block;
        return;
    }

    // Find insertion point (keep list sorted by address)
    Block* current = free_list;
    while (current->next && current->next < block) {
        current = current->next;
    }

    // Insert after current
    block->next = current->next;
    block->prev = current;
    if (current->next) {
        current->next->prev = block;
    }
    current->next = block;
}

// ============================================================================
// COALESCING
// ============================================================================

// Coalesce adjacent free blocks
static Block* coalesce(Block* block) {
    // Get next physical block
    Block* next = (Block*)((uint8_t*)block + block->size);

    // Check if next block exists and is free
    if (is_valid_heap_ptr(next) &&
        (uint8_t*)next < heap + HEAP_SIZE &&
        next->is_free) {
        // Merge with next block
        remove_from_free_list(next);
        block->size += next->size;
    }

    // Check if previous block is free (requires scanning free list)
    if (block->prev && block->prev->is_free) {
        Block* prev_phys = (Block*)((uint8_t*)block->prev + block->prev->size);
        if (prev_phys == block) {
            // Merge with previous block
            block->prev->size += block->size;
            remove_from_free_list(block);
            return block->prev;
        }
    }

    return block;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

static void init_heap() {
    debug_print("[malloc] init_heap() START\n");

    if (heap_initialized) {
        debug_print("[malloc] heap already initialized\n");
        return;
    }

    debug_print("[malloc] Getting heap pointer...\n");
    // Initialize first block to cover entire heap
    Block* initial_block = (Block*)heap;

    debug_print("[malloc] Setting block size...\n");
    initial_block->size = HEAP_SIZE;

    debug_print("[malloc] Setting is_free...\n");
    initial_block->is_free = true;

    debug_print("[malloc] Setting next/prev...\n");
    initial_block->next = NULL;
    initial_block->prev = NULL;

    debug_print("[malloc] Setting free_list...\n");
    free_list = initial_block;

    debug_print("[malloc] Setting heap_initialized...\n");
    heap_initialized = true;

    debug_print("[malloc] Resetting stats...\n");
    // Reset statistics
    total_allocated = 0;
    total_freed = 0;
    current_usage = 0;
    peak_usage = 0;
    num_allocations = 0;
    num_frees = 0;

    debug_print("[malloc] init_heap() DONE\n");
}

// ============================================================================
// ALLOCATION
// ============================================================================

void* malloc(size_t size) {
    debug_print("[malloc] malloc() called\n");

    if (size == 0) {
        debug_print("[malloc] size == 0, returning NULL\n");
        return NULL;
    }

    debug_print("[malloc] Checking heap_initialized...\n");
    if (!heap_initialized) {
        debug_print("[malloc] Calling init_heap()...\n");
        init_heap();
        debug_print("[malloc] init_heap() returned\n");
    }

    // Align size and add header
    size_t aligned_size = align_size(size);
    size_t total_size = aligned_size + BLOCK_HEADER_SIZE;

    // Ensure minimum block size
    if (total_size < MIN_BLOCK_SIZE) {
        total_size = MIN_BLOCK_SIZE;
    }

    // Search free list for suitable block (first-fit)
    Block* current = free_list;
    while (current) {
        if (current->is_free && current->size >= total_size) {
            // Found suitable block
            break;
        }
        current = current->next;
    }

    if (!current) {
        // No suitable block found
        return NULL;
    }

    // Split block if remainder is large enough
    size_t remainder = current->size - total_size;
    if (remainder >= MIN_BLOCK_SIZE) {
        // Split the block
        Block* new_block = (Block*)((uint8_t*)current + total_size);
        new_block->size = remainder;
        new_block->is_free = true;
        new_block->next = current->next;
        new_block->prev = current->prev;

        // Update current block
        current->size = total_size;

        // Replace current in free list with new_block
        if (current->prev) {
            current->prev->next = new_block;
        } else {
            free_list = new_block;
        }
        if (current->next) {
            current->next->prev = new_block;
        }
    } else {
        // Use entire block
        remove_from_free_list(current);
    }

    // Mark block as allocated
    current->is_free = false;
    current->next = NULL;
    current->prev = NULL;

    // Update statistics
    total_allocated += size;
    current_usage += current->size;
    if (current_usage > peak_usage) {
        peak_usage = current_usage;
    }
    num_allocations++;

    return block_to_ptr(current);
}

// ============================================================================
// DEALLOCATION
// ============================================================================

void free(void* ptr) {
    if (!ptr || !is_valid_heap_ptr(ptr)) {
        return;
    }

    Block* block = ptr_to_block(ptr);

    if (block->is_free) {
        // Double free - should not happen
        return;
    }

    // Update statistics
    total_freed += block->size - BLOCK_HEADER_SIZE;
    current_usage -= block->size;
    num_frees++;

    // Add to free list
    add_to_free_list(block);

    // Coalesce with adjacent free blocks
    coalesce(block);
}

// ============================================================================
// OTHER ALLOCATION FUNCTIONS
// ============================================================================

void* calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    Block* block = ptr_to_block(ptr);
    size_t old_size = block->size - BLOCK_HEADER_SIZE;

    if (old_size >= size) {
        // Current block is large enough
        return ptr;
    }

    // Allocate new block
    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data
    memcpy(new_ptr, ptr, old_size);

    // Free old block
    free(ptr);

    return new_ptr;
}

// ============================================================================
// DEBUGGING / STATISTICS
// ============================================================================

void malloc_stats() {
    // This would print stats, but we'll stub it for now
    // In bare-metal, use serial_printf
}

size_t malloc_get_usage() {
    return current_usage;
}

size_t malloc_get_peak() {
    return peak_usage;
}

size_t malloc_get_heap_size() {
    return HEAP_SIZE;
}
