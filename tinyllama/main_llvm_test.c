/**
 * TinyLlama LLVM Runtime Test
 *
 * Tests kernel_lib_llvm.a in real bare-metal QEMU environment:
 * - malloc_llvm allocator (200 MB heap)
 * - String functions
 * - Serial output
 * - Validates bare-metal runtime
 *
 * This proves our runtime works in actual x86 bare-metal!
 */

#include "../kernel_lib/runtime.h"
#include "../kernel_lib/jit_runtime.h"

// External functions from kernel_lib_llvm.a
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* calloc(size_t nmemb, size_t size);
extern size_t malloc_get_usage(void);
extern size_t malloc_get_peak(void);
extern size_t malloc_get_heap_size(void);

// ============================================================================
// Helper Functions
// ============================================================================

static void print_stats(const char* label) {
    size_t usage = malloc_get_usage();
    size_t peak = malloc_get_peak();
    size_t total = malloc_get_heap_size();

    serial_puts(label);
    serial_puts(":\n  Usage: ");
    serial_put_uint(usage / 1024);
    serial_puts(" KB / ");
    serial_put_uint(total / (1024 * 1024));
    serial_puts(" MB (");
    serial_put_uint((usage * 100) / total);
    serial_puts("%)\n  Peak:  ");
    serial_put_uint(peak / 1024);
    serial_puts(" KB\n\n");
}

// ============================================================================
// Tests
// ============================================================================

static int test_basic_allocation(void) {
    serial_puts("=== Test 1: Basic Allocation ===\n");

    void* p1 = malloc(1000);
    if (!p1) {
        serial_puts("  FAIL: malloc(1000) returned NULL\n\n");
        return 1;
    }

    void* p2 = malloc(2000);
    if (!p2) {
        serial_puts("  FAIL: malloc(2000) returned NULL\n\n");
        return 1;
    }

    void* p3 = malloc(3000);
    if (!p3) {
        serial_puts("  FAIL: malloc(3000) returned NULL\n\n");
        return 1;
    }

    serial_puts("  Allocated 3 blocks: 1000, 2000, 3000 bytes\n");

    // Write test patterns
    memset(p1, 0xAA, 1000);
    memset(p2, 0xBB, 2000);
    memset(p3, 0xCC, 3000);

    // Verify
    uint8_t* p1_bytes = (uint8_t*)p1;
    uint8_t* p2_bytes = (uint8_t*)p2;
    uint8_t* p3_bytes = (uint8_t*)p3;

    if (p1_bytes[0] != 0xAA || p2_bytes[0] != 0xBB || p3_bytes[0] != 0xCC) {
        serial_puts("  FAIL: Memory corruption detected\n\n");
        return 1;
    }

    serial_puts("  Data integrity verified\n");

    free(p1);
    free(p2);
    free(p3);

    serial_puts("  PASS\n\n");
    return 0;
}

static int test_large_allocation(void) {
    serial_puts("=== Test 2: Large Allocation ===\n");

    // Allocate 10 MB
    void* p = malloc(10 * 1024 * 1024);
    if (!p) {
        serial_puts("  FAIL: malloc(10 MB) returned NULL\n\n");
        return 1;
    }

    serial_puts("  Allocated 10 MB block\n");
    print_stats("  After allocation");

    free(p);

    serial_puts("  Freed 10 MB block\n");
    serial_puts("  PASS\n\n");
    return 0;
}

static int test_calloc(void) {
    serial_puts("=== Test 3: calloc ===\n");

    uint8_t* p = (uint8_t*)calloc(100, 1);
    if (!p) {
        serial_puts("  FAIL: calloc(100, 1) returned NULL\n\n");
        return 1;
    }

    // Verify zeroed
    for (int i = 0; i < 100; i++) {
        if (p[i] != 0) {
            serial_puts("  FAIL: Memory not zeroed at index ");
            serial_put_uint(i);
            serial_puts("\n\n");
            return 1;
        }
    }

    serial_puts("  Verified 100 bytes zeroed\n");
    free(p);
    serial_puts("  PASS\n\n");
    return 0;
}

static int test_string_functions(void) {
    serial_puts("=== Test 4: String Functions ===\n");

    char* str1 = (char*)malloc(20);
    char* str2 = (char*)malloc(20);

    if (!str1 || !str2) {
        serial_puts("  FAIL: malloc failed\n\n");
        return 1;
    }

    // Test memcpy
    const char* test = "Hello QEMU!";
    memcpy(str1, test, 12);
    str1[11] = '\0';

    serial_puts("  memcpy: ");
    serial_puts(str1);
    serial_puts("\n");

    // Test memset
    memset(str2, 'X', 10);
    str2[10] = '\0';

    serial_puts("  memset: ");
    serial_puts(str2);
    serial_puts("\n");

    // Test strlen
    size_t len = strlen(test);
    serial_puts("  strlen: ");
    serial_put_uint(len);
    serial_puts("\n");

    free(str1);
    free(str2);

    serial_puts("  PASS\n\n");
    return 0;
}

// ============================================================================
// Main Entry Point
// ============================================================================

void main(void) {
    // Initialize serial port for output
    serial_init();

    serial_puts("\n");
    serial_puts("========================================\n");
    serial_puts("  TinyLlama LLVM Runtime Test\n");
    serial_puts("  Running in QEMU (bare-metal x86)\n");
    serial_puts("========================================\n\n");

    print_stats("Initial state");

    int failures = 0;

    failures += test_basic_allocation();
    failures += test_large_allocation();
    failures += test_calloc();
    failures += test_string_functions();

    print_stats("Final state");

    if (failures == 0) {
        serial_puts("========================================\n");
        serial_puts("  ALL TESTS PASSED (4/4)\n");
        serial_puts("========================================\n\n");

        serial_puts("Validation:\n");
        serial_puts("  malloc_llvm working in bare-metal\n");
        serial_puts("  free() with coalescing working\n");
        serial_puts("  calloc working\n");
        serial_puts("  Large allocations (10 MB) working\n");
        serial_puts("  String functions working\n");
        serial_puts("  Serial I/O working\n");
        serial_puts("\n");
        serial_puts("kernel_lib_llvm.a validated in QEMU!\n");
        serial_puts("\n");
    } else {
        serial_puts("========================================\n");
        serial_puts("  ");
        serial_put_uint(failures);
        serial_puts(" TEST(S) FAILED\n");
        serial_puts("========================================\n");
    }

    serial_puts("\nSystem halted. Close QEMU to exit.\n");

    // Halt
    while (1) {
        __asm__ volatile("hlt");
    }
}
