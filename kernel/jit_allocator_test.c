/**
 * JIT Allocator Test Suite
 */

#include "jit_allocator_test.h"
#include "jit_allocator.h"
#include "vga.h"

// External functions
extern void* memset(void* s, int c, size_t n);

// ============================================================================
// Test Helpers
// ============================================================================

static int g_tests_passed = 0;
static int g_tests_total = 0;

#define TEST_START(name) \
    terminal_writestring("\n[Test] "); \
    terminal_writestring(name); \
    terminal_writestring("\n"); \
    g_tests_total++;

#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        terminal_writestring("  FAIL: "); \
        terminal_writestring(message); \
        terminal_writestring("\n"); \
        return 0; \
    }

#define TEST_PASS() \
    terminal_writestring("  PASS\n"); \
    g_tests_passed++; \
    return 1;

// ============================================================================
// Test Cases
// ============================================================================

static int test_init_shutdown(void) {
    TEST_START("Allocator initialization and shutdown");

    // Use larger pool sizes with more RAM available (64MB)
    int result = jit_allocator_init(256 * 1024, 512 * 1024, 128 * 1024);
    TEST_ASSERT(result == 0, "Initialization failed");

    jit_allocator_shutdown();

    // Re-initialize for other tests
    result = jit_allocator_init(256 * 1024, 512 * 1024, 128 * 1024);
    TEST_ASSERT(result == 0, "Re-initialization failed");

    TEST_PASS();
}

static int test_simple_allocation(void) {
    TEST_START("Simple allocation and free");

    void* ptr = jit_alloc(1024, JIT_POOL_DATA, 0);
    TEST_ASSERT(ptr != NULL, "Allocation returned NULL");

    // Write to memory to ensure it's accessible
    memset(ptr, 0xAA, 1024);

    jit_free(ptr, JIT_POOL_DATA);

    TEST_PASS();
}

static int test_multiple_allocations(void) {
    TEST_START("Multiple allocations");

    void* ptrs[10];

    for (int i = 0; i < 10; i++) {
        ptrs[i] = jit_alloc(256, JIT_POOL_DATA, 0);
        TEST_ASSERT(ptrs[i] != NULL, "Allocation failed");
    }

    // Free in reverse order
    for (int i = 9; i >= 0; i--) {
        jit_free(ptrs[i], JIT_POOL_DATA);
    }

    TEST_PASS();
}

static int test_aligned_allocation(void) {
    TEST_START("Aligned allocation");

    void* ptr = jit_alloc_aligned(512, 64, JIT_POOL_CODE, 0);
    TEST_ASSERT(ptr != NULL, "Aligned allocation returned NULL");

    // Check alignment
    uintptr_t addr = (uintptr_t)ptr;
    TEST_ASSERT((addr % 64) == 0, "Pointer is not 64-byte aligned");

    jit_free(ptr, JIT_POOL_CODE);

    TEST_PASS();
}

static int test_zeroed_allocation(void) {
    TEST_START("Zeroed allocation");

    void* ptr = jit_alloc(512, JIT_POOL_DATA, JIT_ALLOC_ZEROED);
    TEST_ASSERT(ptr != NULL, "Allocation failed");

    // Check that memory is zeroed
    uint8_t* bytes = (uint8_t*)ptr;
    for (int i = 0; i < 512; i++) {
        TEST_ASSERT(bytes[i] == 0, "Memory not zeroed");
    }

    jit_free(ptr, JIT_POOL_DATA);

    TEST_PASS();
}

static int test_realloc(void) {
    TEST_START("Reallocation");

    void* ptr = jit_alloc(256, JIT_POOL_DATA, 0);
    TEST_ASSERT(ptr != NULL, "Initial allocation failed");

    // Fill with pattern
    uint8_t* bytes = (uint8_t*)ptr;
    for (int i = 0; i < 256; i++) {
        bytes[i] = (uint8_t)(i & 0xFF);
    }

    // Reallocate larger
    void* new_ptr = jit_realloc(ptr, 512, JIT_POOL_DATA, 0);
    TEST_ASSERT(new_ptr != NULL, "Reallocation failed");

    // Check that data was preserved
    bytes = (uint8_t*)new_ptr;
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT(bytes[i] == (uint8_t)(i & 0xFF), "Data not preserved after realloc");
    }

    jit_free(new_ptr, JIT_POOL_DATA);

    TEST_PASS();
}

static int test_pool_statistics(void) {
    TEST_START("Pool statistics");

    jit_pool_stats_t stats_before, stats_after;
    jit_get_pool_stats(JIT_POOL_DATA, &stats_before);

    void* ptr = jit_alloc(2048, JIT_POOL_DATA, 0);
    TEST_ASSERT(ptr != NULL, "Allocation failed");

    jit_get_pool_stats(JIT_POOL_DATA, &stats_after);

    // Check that statistics were updated
    TEST_ASSERT(stats_after.used_size > stats_before.used_size, "Used size not increased");
    TEST_ASSERT(stats_after.num_allocations > stats_before.num_allocations, "Allocation count not increased");

    jit_free(ptr, JIT_POOL_DATA);

    TEST_PASS();
}

static int test_different_pools(void) {
    TEST_START("Allocation from different pools");

    terminal_writestring("  Allocating from CODE pool...\n");
    void* code_ptr = jit_alloc(512, JIT_POOL_CODE, 0);
    TEST_ASSERT(code_ptr != NULL, "CODE pool allocation failed");

    terminal_writestring("  Allocating from DATA pool...\n");
    void* data_ptr = jit_alloc(512, JIT_POOL_DATA, 0);
    TEST_ASSERT(data_ptr != NULL, "DATA pool allocation failed");

    terminal_writestring("  Allocating from METADATA pool...\n");
    void* meta_ptr = jit_alloc(512, JIT_POOL_METADATA, 0);
    TEST_ASSERT(meta_ptr != NULL, "METADATA pool allocation failed");

    // Verify pointers are from correct pools
    jit_pool_type_t pool;
    TEST_ASSERT(jit_is_pool_pointer(code_ptr, &pool) && pool == JIT_POOL_CODE,
                "CODE pointer not in CODE pool");
    TEST_ASSERT(jit_is_pool_pointer(data_ptr, &pool) && pool == JIT_POOL_DATA,
                "DATA pointer not in DATA pool");
    TEST_ASSERT(jit_is_pool_pointer(meta_ptr, &pool) && pool == JIT_POOL_METADATA,
                "METADATA pointer not in METADATA pool");

    jit_free(code_ptr, JIT_POOL_CODE);
    jit_free(data_ptr, JIT_POOL_DATA);
    jit_free(meta_ptr, JIT_POOL_METADATA);

    TEST_PASS();
}

static int test_fragmentation(void) {
    TEST_START("Fragmentation and coalescence");

    void* ptrs[5];

    // Allocate 5 blocks
    for (int i = 0; i < 5; i++) {
        ptrs[i] = jit_alloc(512, JIT_POOL_DATA, 0);
        TEST_ASSERT(ptrs[i] != NULL, "Allocation failed");
    }

    // Free every other block
    jit_free(ptrs[1], JIT_POOL_DATA);
    jit_free(ptrs[3], JIT_POOL_DATA);

    // Should be able to allocate in freed space
    void* new_ptr = jit_alloc(512, JIT_POOL_DATA, 0);
    TEST_ASSERT(new_ptr != NULL, "Allocation in fragmented space failed");

    // Cleanup
    jit_free(ptrs[0], JIT_POOL_DATA);
    jit_free(ptrs[2], JIT_POOL_DATA);
    jit_free(ptrs[4], JIT_POOL_DATA);
    jit_free(new_ptr, JIT_POOL_DATA);

    TEST_PASS();
}

static int test_pool_reset(void) {
    TEST_START("Pool reset");

    // Allocate several blocks
    void* ptr1 = jit_alloc(1024, JIT_POOL_METADATA, 0);
    void* ptr2 = jit_alloc(2048, JIT_POOL_METADATA, 0);
    TEST_ASSERT(ptr1 != NULL && ptr2 != NULL, "Allocations failed");

    jit_pool_stats_t stats_before;
    jit_get_pool_stats(JIT_POOL_METADATA, &stats_before);
    TEST_ASSERT(stats_before.used_size > 0, "No memory used before reset");

    // Reset pool
    jit_reset_pool(JIT_POOL_METADATA);

    jit_pool_stats_t stats_after;
    jit_get_pool_stats(JIT_POOL_METADATA, &stats_after);
    TEST_ASSERT(stats_after.used_size == 0, "Pool not reset");

    TEST_PASS();
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int test_jit_allocator(void) {
    terminal_writestring("\n");
    terminal_writestring("========================================\n");
    terminal_writestring("  JIT Allocator Test Suite\n");
    terminal_writestring("========================================\n");

    g_tests_passed = 0;
    g_tests_total = 0;

    test_init_shutdown();
    test_simple_allocation();
    test_multiple_allocations();
    test_aligned_allocation();
    test_zeroed_allocation();
    test_realloc();
    test_pool_statistics();
    test_different_pools();
    test_fragmentation();
    test_pool_reset();

    terminal_writestring("\n========================================\n");
    terminal_writestring("  Results: ");

    // Print results (simple version)
    char passed_str[16] = {0};
    char total_str[16] = {0};

    int p = g_tests_passed;
    int idx = 0;
    if (p == 0) {
        passed_str[idx++] = '0';
    } else {
        while (p > 0) {
            passed_str[idx++] = '0' + (p % 10);
            p /= 10;
        }
    }
    // Reverse
    for (int i = 0; i < idx / 2; i++) {
        char tmp = passed_str[i];
        passed_str[i] = passed_str[idx - 1 - i];
        passed_str[idx - 1 - i] = tmp;
    }
    terminal_writestring(passed_str);

    terminal_writestring(" / ");

    int t = g_tests_total;
    idx = 0;
    if (t == 0) {
        total_str[idx++] = '0';
    } else {
        while (t > 0) {
            total_str[idx++] = '0' + (t % 10);
            t /= 10;
        }
    }
    // Reverse
    for (int i = 0; i < idx / 2; i++) {
        char tmp = total_str[i];
        total_str[i] = total_str[idx - 1 - i];
        total_str[idx - 1 - i] = tmp;
    }
    terminal_writestring(total_str);

    terminal_writestring(" tests passed\n");
    terminal_writestring("========================================\n\n");

    return (g_tests_passed == g_tests_total) ? 0 : 1;
}
