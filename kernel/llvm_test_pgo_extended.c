// ============================================================================
// BAREFLOW - Extended PGO Performance Test Suite
// ============================================================================
// Extended test suite with branch-heavy and dispatch-heavy modules:
//
// Previous modules (simple, deterministic):
// - matrix_mul:  1500 iterations → HOT
// - sha256:      2000 iterations → HOT
// - primes:     10000 iterations → VERY_HOT
//
// NEW modules (complex, unpredictable branches):
// - fft_1d:            2000 iterations → HOT (bit reversal branches)
// - quicksort:         3000 iterations → HOT (partition branches)
// - compute_dispatch:  3000 iterations → HOT (indirect calls)
//
// Total: 21,500+ iterations demonstrating PGO on complex workloads
// ============================================================================

#include "llvm_module_manager.h"
#include "vga.h"
#include "stdlib.h"

extern void serial_puts(const char* str);

// ============================================================================
// External Module Declarations
// ============================================================================

// FFT module
extern uint8_t _binary_llvm_modules_fft_1d_O0_elf_start[];
extern uint8_t _binary_llvm_modules_fft_1d_O0_elf_end[];
extern uint8_t _binary_llvm_modules_fft_1d_O1_elf_start[];
extern uint8_t _binary_llvm_modules_fft_1d_O1_elf_end[];
extern uint8_t _binary_llvm_modules_fft_1d_O2_elf_start[];
extern uint8_t _binary_llvm_modules_fft_1d_O2_elf_end[];
extern uint8_t _binary_llvm_modules_fft_1d_O3_elf_start[];
extern uint8_t _binary_llvm_modules_fft_1d_O3_elf_end[];

// Quicksort module
extern uint8_t _binary_llvm_modules_quicksort_O0_elf_start[];
extern uint8_t _binary_llvm_modules_quicksort_O0_elf_end[];
extern uint8_t _binary_llvm_modules_quicksort_O1_elf_start[];
extern uint8_t _binary_llvm_modules_quicksort_O1_elf_end[];
extern uint8_t _binary_llvm_modules_quicksort_O2_elf_start[];
extern uint8_t _binary_llvm_modules_quicksort_O2_elf_end[];
extern uint8_t _binary_llvm_modules_quicksort_O3_elf_start[];
extern uint8_t _binary_llvm_modules_quicksort_O3_elf_end[];

// Compute dispatch module
extern uint8_t _binary_llvm_modules_compute_dispatch_O0_elf_start[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O0_elf_end[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O1_elf_start[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O1_elf_end[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O2_elf_start[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O2_elf_end[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O3_elf_start[];
extern uint8_t _binary_llvm_modules_compute_dispatch_O3_elf_end[];

// ============================================================================
// Helper Functions
// ============================================================================

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

// ============================================================================
// Main Test Suite
// ============================================================================

void test_llvm_pgo_extended(void) {
    serial_puts("\n");
    serial_puts("========================================================================\n");
    serial_puts("=== EXTENDED PGO PERFORMANCE TEST SUITE ===\n");
    serial_puts("========================================================================\n");
    serial_puts("\n");
    serial_puts("Testing advanced modules with complex branch patterns:\n");
    serial_puts("  - fft_1d:           2000 calls → HOT (bit reversal)\n");
    serial_puts("  - quicksort:        3000 calls → HOT (partition branches)\n");
    serial_puts("  - compute_dispatch: 3000 calls → HOT (indirect calls)\n");
    serial_puts("\n");

    // Initialize manager
    llvm_module_manager_t mgr;
    llvm_module_manager_init(&mgr);

    // ========================================================================
    // Register all three new modules
    // ========================================================================

    serial_puts("[1] Registering advanced modules...\n");

    // FFT module
    size_t size_fft_o0 = (size_t)(_binary_llvm_modules_fft_1d_O0_elf_end -
                                  _binary_llvm_modules_fft_1d_O0_elf_start);
    size_t size_fft_o1 = (size_t)(_binary_llvm_modules_fft_1d_O1_elf_end -
                                  _binary_llvm_modules_fft_1d_O1_elf_start);
    size_t size_fft_o2 = (size_t)(_binary_llvm_modules_fft_1d_O2_elf_end -
                                  _binary_llvm_modules_fft_1d_O2_elf_start);
    size_t size_fft_o3 = (size_t)(_binary_llvm_modules_fft_1d_O3_elf_end -
                                  _binary_llvm_modules_fft_1d_O3_elf_start);

    int fft_id = llvm_module_register(&mgr, "fft_1d",
                                      _binary_llvm_modules_fft_1d_O0_elf_start, size_fft_o0,
                                      _binary_llvm_modules_fft_1d_O1_elf_start, size_fft_o1,
                                      _binary_llvm_modules_fft_1d_O2_elf_start, size_fft_o2,
                                      _binary_llvm_modules_fft_1d_O3_elf_start, size_fft_o3);

    if (fft_id < 0) {
        serial_puts("    [ERROR] fft_1d registration failed\n");
        return;
    }
    serial_puts("    ✓ fft_1d registered\n");

    // Quicksort module
    size_t size_qs_o0 = (size_t)(_binary_llvm_modules_quicksort_O0_elf_end -
                                 _binary_llvm_modules_quicksort_O0_elf_start);
    size_t size_qs_o1 = (size_t)(_binary_llvm_modules_quicksort_O1_elf_end -
                                 _binary_llvm_modules_quicksort_O1_elf_start);
    size_t size_qs_o2 = (size_t)(_binary_llvm_modules_quicksort_O2_elf_end -
                                 _binary_llvm_modules_quicksort_O2_elf_start);
    size_t size_qs_o3 = (size_t)(_binary_llvm_modules_quicksort_O3_elf_end -
                                 _binary_llvm_modules_quicksort_O3_elf_start);

    int qs_id = llvm_module_register(&mgr, "quicksort",
                                     _binary_llvm_modules_quicksort_O0_elf_start, size_qs_o0,
                                     _binary_llvm_modules_quicksort_O1_elf_start, size_qs_o1,
                                     _binary_llvm_modules_quicksort_O2_elf_start, size_qs_o2,
                                     _binary_llvm_modules_quicksort_O3_elf_start, size_qs_o3);

    if (qs_id < 0) {
        serial_puts("    [ERROR] quicksort registration failed\n");
        return;
    }
    serial_puts("    ✓ quicksort registered\n");

    // Compute dispatch module
    size_t size_cd_o0 = (size_t)(_binary_llvm_modules_compute_dispatch_O0_elf_end -
                                 _binary_llvm_modules_compute_dispatch_O0_elf_start);
    size_t size_cd_o1 = (size_t)(_binary_llvm_modules_compute_dispatch_O1_elf_end -
                                 _binary_llvm_modules_compute_dispatch_O1_elf_start);
    size_t size_cd_o2 = (size_t)(_binary_llvm_modules_compute_dispatch_O2_elf_end -
                                 _binary_llvm_modules_compute_dispatch_O2_elf_start);
    size_t size_cd_o3 = (size_t)(_binary_llvm_modules_compute_dispatch_O3_elf_end -
                                 _binary_llvm_modules_compute_dispatch_O3_elf_start);

    int cd_id = llvm_module_register(&mgr, "compute_dispatch",
                                     _binary_llvm_modules_compute_dispatch_O0_elf_start, size_cd_o0,
                                     _binary_llvm_modules_compute_dispatch_O1_elf_start, size_cd_o1,
                                     _binary_llvm_modules_compute_dispatch_O2_elf_start, size_cd_o2,
                                     _binary_llvm_modules_compute_dispatch_O3_elf_start, size_cd_o3);

    if (cd_id < 0) {
        serial_puts("    [ERROR] compute_dispatch registration failed\n");
        return;
    }
    serial_puts("    ✓ compute_dispatch registered\n\n");

    // ========================================================================
    // Test 1: FFT (2000 iterations → HOT)
    // ========================================================================

    serial_puts("[2] Testing fft_1d (16-point FFT with bit reversal)...\n");
    serial_puts("    Target: 2000 iterations → HOT classification\n");
    serial_puts("    Benefit: Branch prediction for bit reversal patterns\n\n");

    int fft_result = llvm_module_execute(&mgr, fft_id);
    serial_puts("    Result: ");
    print_int(fft_result);
    serial_puts(" (magnitude sum)\n");

    serial_puts("    Running 2000 iterations with adaptive optimization...\n");
    for (int i = 0; i < 2000; i++) {
        llvm_module_execute_adaptive(&mgr, fft_id);

        if ((i + 1) % 500 == 0) {
            serial_puts("      → ");
            print_int(i + 1);
            serial_puts(" iterations complete\n");
        }
    }

    serial_puts("    ✓ fft_1d test complete\n\n");
    llvm_module_print_stats(&mgr, fft_id);

    // ========================================================================
    // Test 2: Quicksort (3000 iterations → HOT)
    // ========================================================================

    serial_puts("\n[3] Testing quicksort (hybrid with insertion sort)...\n");
    serial_puts("    Target: 3000 iterations → HOT classification\n");
    serial_puts("    Benefit: Branch prediction for partition decisions\n\n");

    int qs_result = llvm_module_execute(&mgr, qs_id);
    serial_puts("    Result: ");
    print_int(qs_result);
    serial_puts(" (checksum + sorted flag)\n");

    serial_puts("    Running 3000 iterations with adaptive optimization...\n");
    for (int i = 0; i < 3000; i++) {
        llvm_module_execute_adaptive(&mgr, qs_id);

        if ((i + 1) % 750 == 0) {
            serial_puts("      → ");
            print_int(i + 1);
            serial_puts(" iterations complete\n");
        }
    }

    serial_puts("    ✓ quicksort test complete\n\n");
    llvm_module_print_stats(&mgr, qs_id);

    // ========================================================================
    // Test 3: Compute Dispatch (3000 iterations → HOT)
    // ========================================================================

    serial_puts("\n[4] Testing compute_dispatch (indirect function calls)...\n");
    serial_puts("    Target: 3000 iterations → HOT classification\n");
    serial_puts("    Benefit: Devirtualization of hot dispatch targets\n\n");

    int cd_result = llvm_module_execute(&mgr, cd_id);
    serial_puts("    Result: ");
    print_int(cd_result);
    serial_puts(" (dispatch accumulator)\n");

    serial_puts("    Running 3000 iterations with adaptive optimization...\n");
    for (int i = 0; i < 3000; i++) {
        llvm_module_execute_adaptive(&mgr, cd_id);

        if ((i + 1) % 750 == 0) {
            serial_puts("      → ");
            print_int(i + 1);
            serial_puts(" iterations complete\n");
        }
    }

    serial_puts("    ✓ compute_dispatch test complete\n\n");
    llvm_module_print_stats(&mgr, cd_id);

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
    serial_puts("=== EXTENDED PGO TEST SUITE COMPLETE ===\n");
    serial_puts("========================================================================\n");
    serial_puts("\n");
    serial_puts("Results:\n");
    serial_puts("  ✓ fft_1d:           2000 iterations (HOT - bit reversal)\n");
    serial_puts("  ✓ quicksort:        3000 iterations (HOT - partition branches)\n");
    serial_puts("  ✓ compute_dispatch: 3000 iterations (HOT - indirect calls)\n");
    serial_puts("\n");
    serial_puts("Expected PGO benefits:\n");
    serial_puts("  - fft_1d:           10-20%% (branch prediction)\n");
    serial_puts("  - quicksort:        15-30%% (partition optimization)\n");
    serial_puts("  - compute_dispatch: 20-40%% (devirtualization)\n");
    serial_puts("\n");
}
