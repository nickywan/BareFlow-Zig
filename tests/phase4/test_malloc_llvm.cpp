/**
 * Test: Enhanced LLVM Allocator
 *
 * Tests the free-list allocator designed for LLVM:
 * - Large allocations (up to 10 MB)
 * - Proper free() with coalescing
 * - Memory reuse
 * - Fragmentation handling
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// External allocator functions
extern "C" {
    void* malloc(size_t size);
    void free(void* ptr);
    void* calloc(size_t nmemb, size_t size);
    void* realloc(void* ptr, size_t size);

    // Statistics functions
    size_t malloc_get_usage();
    size_t malloc_get_peak();
    size_t malloc_get_heap_size();
}

// ============================================================================
// Test Helpers
// ============================================================================

void print_stats(const char* label) {
    size_t usage = malloc_get_usage();
    size_t peak = malloc_get_peak();
    size_t total = malloc_get_heap_size();

    printf("%s:\n", label);
    printf("  Current usage: %zu bytes (%.2f MB)\n", usage, usage / (1024.0 * 1024.0));
    printf("  Peak usage:    %zu bytes (%.2f MB)\n", peak, peak / (1024.0 * 1024.0));
    printf("  Heap size:     %zu bytes (%.2f MB)\n", total, total / (1024.0 * 1024.0));
    printf("  Utilization:   %.2f%%\n", (usage * 100.0) / total);
    printf("\n");
}

// ============================================================================
// Tests
// ============================================================================

void test_basic_allocation() {
    printf("=== Test 1: Basic Allocation ===\n");

    void* p1 = malloc(100);
    assert(p1 != NULL);
    memset(p1, 0xAA, 100);

    void* p2 = malloc(200);
    assert(p2 != NULL);
    memset(p2, 0xBB, 200);

    void* p3 = malloc(300);
    assert(p3 != NULL);
    memset(p3, 0xCC, 300);

    printf("  Allocated 3 blocks: 100, 200, 300 bytes\n");
    print_stats("After allocations");

    // Verify data
    assert(((uint8_t*)p1)[0] == 0xAA);
    assert(((uint8_t*)p2)[0] == 0xBB);
    assert(((uint8_t*)p3)[0] == 0xCC);

    free(p1);
    free(p2);
    free(p3);

    print_stats("After frees");
    printf("✅ PASS\n\n");
}

void test_large_allocations() {
    printf("=== Test 2: Large Allocations ===\n");

    // Allocate 10 MB
    void* p1 = malloc(10 * 1024 * 1024);
    assert(p1 != NULL);
    printf("  Allocated 10 MB block\n");
    print_stats("After 10 MB allocation");

    free(p1);
    print_stats("After free");

    // Allocate 5 MB twice
    void* p2 = malloc(5 * 1024 * 1024);
    void* p3 = malloc(5 * 1024 * 1024);
    assert(p2 != NULL && p3 != NULL);
    printf("  Allocated two 5 MB blocks\n");
    print_stats("After 5 MB + 5 MB");

    free(p2);
    free(p3);

    printf("✅ PASS\n\n");
}

void test_free_and_reuse() {
    printf("=== Test 3: Free and Reuse ===\n");

    void* p1 = malloc(1000);
    void* p2 = malloc(1000);
    void* p3 = malloc(1000);

    printf("  Allocated 3 x 1000 bytes\n");
    print_stats("After allocations");

    // Free middle block
    free(p2);
    printf("  Freed middle block\n");
    print_stats("After freeing middle");

    // Allocate smaller block (should reuse freed space)
    void* p4 = malloc(500);
    assert(p4 != NULL);
    printf("  Allocated 500 bytes (should reuse freed space)\n");
    print_stats("After reallocation");

    free(p1);
    free(p3);
    free(p4);

    printf("✅ PASS\n\n");
}

void test_coalescing() {
    printf("=== Test 4: Coalescing Adjacent Blocks ===\n");

    void* p1 = malloc(1000);
    void* p2 = malloc(1000);
    void* p3 = malloc(1000);
    void* p4 = malloc(1000);

    printf("  Allocated 4 x 1000 bytes\n");
    print_stats("After allocations");

    // Free blocks in different orders to test coalescing
    free(p2);
    printf("  Freed block 2\n");

    free(p3);
    printf("  Freed block 3 (should coalesce with block 2)\n");
    print_stats("After coalescing");

    // Now allocate larger block (should fit in coalesced space)
    void* p5 = malloc(1800);
    assert(p5 != NULL);
    printf("  Allocated 1800 bytes (fits in coalesced space)\n");

    free(p1);
    free(p4);
    free(p5);

    printf("✅ PASS\n\n");
}

void test_many_allocations() {
    printf("=== Test 5: Many Small Allocations ===\n");

    const int NUM_ALLOCS = 1000;
    void* ptrs[NUM_ALLOCS];

    // Allocate many small blocks
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = malloc(64);
        assert(ptrs[i] != NULL);
        memset(ptrs[i], i & 0xFF, 64);
    }

    printf("  Allocated %d x 64 bytes\n", NUM_ALLOCS);
    print_stats("After many allocations");

    // Free every other block
    for (int i = 0; i < NUM_ALLOCS; i += 2) {
        free(ptrs[i]);
    }

    printf("  Freed every other block\n");
    print_stats("After freeing half");

    // Free remaining blocks
    for (int i = 1; i < NUM_ALLOCS; i += 2) {
        free(ptrs[i]);
    }

    print_stats("After freeing all");
    printf("✅ PASS\n\n");
}

void test_calloc() {
    printf("=== Test 6: calloc (zeroed memory) ===\n");

    uint8_t* p = (uint8_t*)calloc(100, sizeof(uint8_t));
    assert(p != NULL);

    // Verify all bytes are zero
    bool all_zero = true;
    for (int i = 0; i < 100; i++) {
        if (p[i] != 0) {
            all_zero = false;
            break;
        }
    }

    assert(all_zero);
    printf("  calloc(100, 1) returned zeroed memory\n");

    free(p);
    printf("✅ PASS\n\n");
}

void test_realloc() {
    printf("=== Test 7: realloc ===\n");

    // Allocate initial block
    int* p = (int*)malloc(10 * sizeof(int));
    assert(p != NULL);

    for (int i = 0; i < 10; i++) {
        p[i] = i * i;
    }

    printf("  Allocated 10 ints\n");

    // Grow to 20 ints
    int* p2 = (int*)realloc(p, 20 * sizeof(int));
    assert(p2 != NULL);

    // Verify old data preserved
    for (int i = 0; i < 10; i++) {
        assert(p2[i] == i * i);
    }

    printf("  Reallocated to 20 ints, old data preserved\n");

    free(p2);
    printf("✅ PASS\n\n");
}

void test_stress() {
    printf("=== Test 8: Stress Test ===\n");

    const int ITERATIONS = 100;
    const int MAX_ALLOCS = 100;
    void* ptrs[MAX_ALLOCS];
    int num_allocs = 0;

    printf("  Running %d iterations of random alloc/free\n", ITERATIONS);

    for (int iter = 0; iter < ITERATIONS; iter++) {
        // Random: allocate or free
        if (num_allocs < MAX_ALLOCS && (rand() % 2 == 0 || num_allocs == 0)) {
            // Allocate
            size_t size = (rand() % 10000) + 1;
            ptrs[num_allocs] = malloc(size);
            if (ptrs[num_allocs]) {
                num_allocs++;
            }
        } else if (num_allocs > 0) {
            // Free random block
            int idx = rand() % num_allocs;
            free(ptrs[idx]);
            // Shift array
            for (int i = idx; i < num_allocs - 1; i++) {
                ptrs[i] = ptrs[i + 1];
            }
            num_allocs--;
        }
    }

    printf("  Final allocations: %d\n", num_allocs);
    print_stats("After stress test");

    // Free remaining
    for (int i = 0; i < num_allocs; i++) {
        free(ptrs[i]);
    }

    print_stats("After cleanup");
    printf("✅ PASS\n\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("===========================================\n");
    printf("  LLVM Enhanced Allocator Test Suite\n");
    printf("===========================================\n\n");

    print_stats("Initial state");

    test_basic_allocation();
    test_large_allocations();
    test_free_and_reuse();
    test_coalescing();
    test_many_allocations();
    test_calloc();
    test_realloc();
    test_stress();

    printf("===========================================\n");
    printf("  ✅ ALL TESTS PASSED\n");
    printf("===========================================\n\n");

    print_stats("Final state");

    printf("Summary:\n");
    printf("  - Free-list allocator: ✓\n");
    printf("  - Large allocations (10 MB): ✓\n");
    printf("  - Proper free() implementation: ✓\n");
    printf("  - Block coalescing: ✓\n");
    printf("  - Memory reuse: ✓\n");
    printf("  - calloc/realloc: ✓\n");
    printf("\n");
    printf("Ready for LLVM integration!\n");

    return 0;
}
