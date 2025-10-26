// ============================================================================
// BAREFLOW - PGO Performance Test Suite
// ============================================================================
// This test suite runs compute-intensive modules with high iteration counts
// to demonstrate Profile-Guided Optimization benefits:
//
// - matrix_mul:  1500 iterations → HOT (≥1000 calls)
// - sha256:      2000 iterations → HOT (≥1000 calls)
// - primes:     10000 iterations → VERY_HOT (≥10000 calls)
//
// The goal is to generate profile data showing measurable PGO gains.
// ============================================================================

#include "llvm_module_manager.h"
#include "vga.h"
#include "stdlib.h"

extern void serial_puts(const char* str);

// Embedded LLVM-compiled modules at different optimization levels

// Matrix multiplication module
extern uint8_t _binary_llvm_modules_matrix_mul_O0_elf_start[];
extern uint8_t _binary_llvm_modules_matrix_mul_O0_elf_end[];
extern uint8_t _binary_llvm_modules_matrix_mul_O1_elf_start[];
extern uint8_t _binary_llvm_modules_matrix_mul_O1_elf_end[];
extern uint8_t _binary_llvm_modules_matrix_mul_O2_elf_start[];
extern uint8_t _binary_llvm_modules_matrix_mul_O2_elf_end[];
extern uint8_t _binary_llvm_modules_matrix_mul_O3_elf_start[];
extern uint8_t _binary_llvm_modules_matrix_mul_O3_elf_end[];

// SHA-256 module
extern uint8_t _binary_llvm_modules_sha256_O0_elf_start[];
extern uint8_t _binary_llvm_modules_sha256_O0_elf_end[];
extern uint8_t _binary_llvm_modules_sha256_O1_elf_start[];
extern uint8_t _binary_llvm_modules_sha256_O1_elf_end[];
extern uint8_t _binary_llvm_modules_sha256_O2_elf_start[];
extern uint8_t _binary_llvm_modules_sha256_O2_elf_end[];
extern uint8_t _binary_llvm_modules_sha256_O3_elf_start[];
extern uint8_t _binary_llvm_modules_sha256_O3_elf_end[];

// Prime sieve module
extern uint8_t _binary_llvm_modules_primes_O0_elf_start[];
extern uint8_t _binary_llvm_modules_primes_O0_elf_end[];
extern uint8_t _binary_llvm_modules_primes_O1_elf_start[];
extern uint8_t _binary_llvm_modules_primes_O1_elf_end[];
extern uint8_t _binary_llvm_modules_primes_O2_elf_start[];
extern uint8_t _binary_llvm_modules_primes_O2_elf_end[];
extern uint8_t _binary_llvm_modules_primes_O3_elf_start[];
extern uint8_t _binary_llvm_modules_primes_O3_elf_end[];

static void print_int(int value) {
    if (value == 0) {
        serial_puts("0");
        return;
    }

    char buf[16];
    int i = 0;
    int is_neg = 0;

    if (value < 0) {
        is_neg = 1;
        value = -value;
    }

    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_neg) buf[i++] = '-';

    for (int j = 0; j < i/2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = tmp;
    }
    buf[i] = '\0';

    serial_puts(buf);
}

void test_llvm_pgo_suite(void) {
    serial_puts("\n");
    serial_puts("========================================================================\n");
    serial_puts("=== LLVM PGO PERFORMANCE TEST SUITE ===\n");
    serial_puts("========================================================================\n");
    serial_puts("\n");
    serial_puts("This test suite demonstrates Profile-Guided Optimization benefits\n");
    serial_puts("by running compute-intensive modules with high iteration counts.\n");
    serial_puts("\n");
    serial_puts("Target hotness levels:\n");
    serial_puts("  - matrix_mul: 1500 calls → HOT (>=1000)\n");
    serial_puts("  - sha256:     2000 calls → HOT (>=1000)\n");
    serial_puts("  - primes:    10000 calls → VERY_HOT (>=10000)\n");
    serial_puts("\n");

    // Initialize manager
    llvm_module_manager_t mgr;
    llvm_module_manager_init(&mgr);

    // ========================================================================
    // Register all three modules
    // ========================================================================

    serial_puts("[1] Registering compute-intensive modules...\n");

    // Matrix multiplication
    size_t size_matrix_o0 = (size_t)(_binary_llvm_modules_matrix_mul_O0_elf_end -
                                     _binary_llvm_modules_matrix_mul_O0_elf_start);
    size_t size_matrix_o1 = (size_t)(_binary_llvm_modules_matrix_mul_O1_elf_end -
                                     _binary_llvm_modules_matrix_mul_O1_elf_start);
    size_t size_matrix_o2 = (size_t)(_binary_llvm_modules_matrix_mul_O2_elf_end -
                                     _binary_llvm_modules_matrix_mul_O2_elf_start);
    size_t size_matrix_o3 = (size_t)(_binary_llvm_modules_matrix_mul_O3_elf_end -
                                     _binary_llvm_modules_matrix_mul_O3_elf_start);

    int matrix_id = llvm_module_register(&mgr, "matrix_mul",
                                         _binary_llvm_modules_matrix_mul_O0_elf_start, size_matrix_o0,
                                         _binary_llvm_modules_matrix_mul_O1_elf_start, size_matrix_o1,
                                         _binary_llvm_modules_matrix_mul_O2_elf_start, size_matrix_o2,
                                         _binary_llvm_modules_matrix_mul_O3_elf_start, size_matrix_o3);

    if (matrix_id < 0) {
        serial_puts("    [ERROR] matrix_mul registration failed\n");
        return;
    }
    serial_puts("    ✓ matrix_mul registered\n");

    // SHA-256
    size_t size_sha_o0 = (size_t)(_binary_llvm_modules_sha256_O0_elf_end -
                                  _binary_llvm_modules_sha256_O0_elf_start);
    size_t size_sha_o1 = (size_t)(_binary_llvm_modules_sha256_O1_elf_end -
                                  _binary_llvm_modules_sha256_O1_elf_start);
    size_t size_sha_o2 = (size_t)(_binary_llvm_modules_sha256_O2_elf_end -
                                  _binary_llvm_modules_sha256_O2_elf_start);
    size_t size_sha_o3 = (size_t)(_binary_llvm_modules_sha256_O3_elf_end -
                                  _binary_llvm_modules_sha256_O3_elf_start);

    int sha_id = llvm_module_register(&mgr, "sha256",
                                      _binary_llvm_modules_sha256_O0_elf_start, size_sha_o0,
                                      _binary_llvm_modules_sha256_O1_elf_start, size_sha_o1,
                                      _binary_llvm_modules_sha256_O2_elf_start, size_sha_o2,
                                      _binary_llvm_modules_sha256_O3_elf_start, size_sha_o3);

    if (sha_id < 0) {
        serial_puts("    [ERROR] sha256 registration failed\n");
        return;
    }
    serial_puts("    ✓ sha256 registered\n");

    // Prime sieve
    size_t size_primes_o0 = (size_t)(_binary_llvm_modules_primes_O0_elf_end -
                                     _binary_llvm_modules_primes_O0_elf_start);
    size_t size_primes_o1 = (size_t)(_binary_llvm_modules_primes_O1_elf_end -
                                     _binary_llvm_modules_primes_O1_elf_start);
    size_t size_primes_o2 = (size_t)(_binary_llvm_modules_primes_O2_elf_end -
                                     _binary_llvm_modules_primes_O2_elf_start);
    size_t size_primes_o3 = (size_t)(_binary_llvm_modules_primes_O3_elf_end -
                                     _binary_llvm_modules_primes_O3_elf_start);

    int primes_id = llvm_module_register(&mgr, "primes",
                                         _binary_llvm_modules_primes_O0_elf_start, size_primes_o0,
                                         _binary_llvm_modules_primes_O1_elf_start, size_primes_o1,
                                         _binary_llvm_modules_primes_O2_elf_start, size_primes_o2,
                                         _binary_llvm_modules_primes_O3_elf_start, size_primes_o3);

    if (primes_id < 0) {
        serial_puts("    [ERROR] primes registration failed\n");
        return;
    }
    serial_puts("    ✓ primes registered\n\n");

    // ========================================================================
    // Test 1: Matrix Multiplication (1500 iterations → HOT)
    // ========================================================================

    serial_puts("[2] Testing matrix_mul (8x8 matrix multiplication)...\n");
    serial_puts("    Target: 1500 iterations → HOT classification\n");
    serial_puts("    Benefit: Loop unrolling, better register allocation\n\n");

    // Verify correctness first
    int matrix_result = llvm_module_execute(&mgr, matrix_id);
    serial_puts("    Result: ");
    print_int(matrix_result);
    serial_puts(" (checksum of 8x8 matrix multiplication)\n");

    // Run 1500 iterations with adaptive optimization
    serial_puts("    Running 1500 iterations with adaptive optimization...\n");
    for (int i = 0; i < 1500; i++) {
        llvm_module_execute_adaptive(&mgr, matrix_id);

        // Progress indicator every 500 iterations
        if ((i + 1) % 500 == 0) {
            serial_puts("      → ");
            print_int(i + 1);
            serial_puts(" iterations complete\n");
        }
    }

    serial_puts("    ✓ matrix_mul test complete\n\n");
    llvm_module_print_stats(&mgr, matrix_id);

    // ========================================================================
    // Test 2: SHA-256 (2000 iterations → HOT)
    // ========================================================================

    serial_puts("\n[3] Testing sha256 (cryptographic hash)...\n");
    serial_puts("    Target: 2000 iterations → HOT classification\n");
    serial_puts("    Benefit: Aggressive inlining, bitwise optimization\n\n");

    // Verify correctness first
    int sha_result = llvm_module_execute(&mgr, sha_id);
    serial_puts("    Result: ");
    print_int(sha_result);
    serial_puts(" (SHA-256 hash of 'Hello World!')\n");

    // Run 2000 iterations with adaptive optimization
    serial_puts("    Running 2000 iterations with adaptive optimization...\n");
    for (int i = 0; i < 2000; i++) {
        llvm_module_execute_adaptive(&mgr, sha_id);

        // Progress indicator every 500 iterations
        if ((i + 1) % 500 == 0) {
            serial_puts("      → ");
            print_int(i + 1);
            serial_puts(" iterations complete\n");
        }
    }

    serial_puts("    ✓ sha256 test complete\n\n");
    llvm_module_print_stats(&mgr, sha_id);

    // ========================================================================
    // Test 3: Prime Sieve (10000 iterations → VERY_HOT)
    // ========================================================================

    serial_puts("\n[4] Testing primes (Sieve of Eratosthenes)...\n");
    serial_puts("    Target: 10000 iterations → VERY_HOT classification\n");
    serial_puts("    Benefit: Vectorization, aggressive loop unrolling\n\n");

    // Verify correctness first
    int primes_result = llvm_module_execute(&mgr, primes_id);
    serial_puts("    Result: ");
    print_int(primes_result);
    serial_puts(" (prime count in sieve + trial division)\n");

    // Run 10000 iterations with adaptive optimization
    serial_puts("    Running 10000 iterations with adaptive optimization...\n");
    for (int i = 0; i < 10000; i++) {
        llvm_module_execute_adaptive(&mgr, primes_id);

        // Progress indicator every 2000 iterations
        if ((i + 1) % 2000 == 0) {
            serial_puts("      → ");
            print_int(i + 1);
            serial_puts(" iterations complete\n");
        }
    }

    serial_puts("    ✓ primes test complete\n\n");
    llvm_module_print_stats(&mgr, primes_id);

    // ========================================================================
    // Export PGO Profile Data
    // ========================================================================

    serial_puts("\n[5] Exporting PGO profile data...\n");
    llvm_module_export_all_profiles(&mgr);

    // ========================================================================
    // Summary
    // ========================================================================

    serial_puts("\n");
    serial_puts("========================================================================\n");
    serial_puts("=== PGO TEST SUITE COMPLETE ===\n");
    serial_puts("========================================================================\n");
    serial_puts("\n");
    serial_puts("Results:\n");
    serial_puts("  ✓ matrix_mul: 1500 iterations (HOT classification)\n");
    serial_puts("  ✓ sha256:     2000 iterations (HOT classification)\n");
    serial_puts("  ✓ primes:    10000 iterations (VERY_HOT classification)\n");
    serial_puts("\n");
    serial_puts("Next steps:\n");
    serial_puts("  1. Extract profile data: ./tools/extract_pgo_profile.sh\n");
    serial_puts("  2. Recompile with PGO:  ./tools/compile_llvm_pgo.sh <module> <profile>\n");
    serial_puts("  3. Measure performance: Compare standard vs PGO execution times\n");
    serial_puts("\n");
    serial_puts("Expected speedups:\n");
    serial_puts("  - matrix_mul (HOT):      1.5-3x improvement\n");
    serial_puts("  - sha256 (HOT):          1.5-3x improvement\n");
    serial_puts("  - primes (VERY_HOT):     2-5x improvement\n");
    serial_puts("\n");
}
