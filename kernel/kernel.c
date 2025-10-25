// ============================================================================
// BAREFLOW KERNEL - With Dynamic Module System
// ============================================================================
// File: kernel/kernel.c
// Entry: kernel_main() is called from entry.asm
// Features: Dynamic module loading, profiling with rdtsc, LLVM-optimized code
// ============================================================================

#include <stddef.h>
#include "vga.h"
#include "module_loader.h"
#include "embedded_modules.h"
#include "cache_loader.h"
#include "keyboard.h"
#include "cxx_runtime.h"
#include "cxx_test.h"
#include "jit_allocator_test.h"
#include "profiling_export.h"
#include "fat16_test.h"
#include "disk_module_loader.h"
#include "micro_jit.h"
#include "jit_allocator.h"
#include "adaptive_jit.h"
#include "jit_demo.h"
#include "elf_test.h"
#include "llvm_test.h"

// Forward declarations
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);

// ============================================================================
// HELPER: VGA print with color (temporary color change)
// ============================================================================
static void vga_print_color(const char* str, enum vga_color fg, enum vga_color bg) {
    // Save current color, set new color, print, restore color
    // Since vga.c doesn't expose current color, we'll use a simpler approach:
    // set color, print, then reset to default
    terminal_setcolor(fg, bg);
    terminal_writestring(str);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);  // Default color
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void print_int(int num) {
    if (num == 0) {
        terminal_putchar('0');
        return;
    }

    if (num < 0) {
        terminal_putchar('-');
        num = -num;
    }

    char buffer[12];
    int i = 0;

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        terminal_putchar(buffer[j]);
    }
}

void print_hex(unsigned int num) {
    terminal_writestring("0x");

    char hex[] = "0123456789ABCDEF";
    char buffer[9];

    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex[num & 0xF];
        num >>= 4;
    }
    buffer[8] = '\0';

    terminal_writestring(buffer);
}

// Helper to wait for user input
void pause_for_key(void) {
#ifdef INTERACTIVE_MODE
    terminal_setcolor(VGA_DARK_GREY, VGA_BLACK);
    terminal_writestring("\n[Press any key to continue...]\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    wait_key();
#endif
}

// ============================================================================
// KERNEL BANNER
// ============================================================================
void print_banner(void) {
    vga_print_color("================================================================================\n",
                    VGA_CYAN, VGA_BLACK);
    vga_print_color("                             FLUID KERNEL v1.0                                 \n",
                    VGA_LIGHT_CYAN, VGA_BLACK);
    vga_print_color("================================================================================\n",
                    VGA_CYAN, VGA_BLACK);
    terminal_writestring("\n");
}

// ============================================================================
// SYSTEM INFO
// ============================================================================
void print_system_info(void) {
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("System Information:\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  - Architecture:     ");
    vga_print_color("x86 (32-bit protected mode)\n", VGA_GREEN, VGA_BLACK);

    terminal_writestring("  - Kernel Address:   ");
    print_hex(0x1000);
    terminal_writestring("\n");

    terminal_writestring("  - VGA Buffer:       ");
    print_hex(0xB8000);
    terminal_writestring("\n");

    terminal_writestring("  - Signature Check:  ");
    vga_print_color("PASSED (FLUD)\n", VGA_GREEN, VGA_BLACK);

    terminal_writestring("\n");
}

// ============================================================================
// CPU INFO
// ============================================================================
void check_cpu_features(void) {
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("CPU Features:\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    unsigned int eax, ebx, ecx, edx;

    // Get CPU vendor
    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    terminal_writestring("  - CPU Vendor:       ");
    char vendor[13];
    *((unsigned int*)&vendor[0]) = ebx;
    *((unsigned int*)&vendor[4]) = edx;
    *((unsigned int*)&vendor[8]) = ecx;
    vendor[12] = '\0';
    terminal_writestring(vendor);
    terminal_writestring("\n");

    // Get CPU features
    asm volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    terminal_writestring("  - FPU:              ");
    terminal_writestring((edx & (1 << 0)) ? "Yes" : "No");
    terminal_writestring("\n");

    terminal_writestring("  - MMX:              ");
    terminal_writestring((edx & (1 << 23)) ? "Yes" : "No");
    terminal_writestring("\n");

    terminal_writestring("  - SSE:              ");
    terminal_writestring((edx & (1 << 25)) ? "Yes" : "No");
    terminal_writestring("\n");

    terminal_writestring("\n");
}

// ============================================================================
// KERNEL MAIN
// ============================================================================
void kernel_main(void) {
    // Initialize VGA terminal
    terminal_initialize();

    // VGA output
    terminal_writestring("Fluid OS - Adaptive JIT System\n\n");

    // Serial output (captured by QEMU)
    serial_puts("\n=== ADAPTIVE JIT DEMONSTRATION ===\n");

    // Initialize JIT allocator (required for micro-JIT)
    // Pools: 32KB code, 32KB data, 16KB metadata (total 80KB, conservative allocation)
    serial_puts("[1] Initializing JIT allocator...\n");
    int jit_init_result = jit_allocator_init(32 * 1024, 32 * 1024, 16 * 1024);
    if (jit_init_result != 0) {
        serial_puts("[1] JIT allocator init FAILED\n");
        goto skip_jit_test;
    }
    serial_puts("[1] JIT allocator initialized OK\n");

    // Initialize adaptive JIT system
    adaptive_jit_t ajit;
    if (adaptive_jit_init(&ajit) != 0) {
        serial_puts("[2] Adaptive JIT init FAILED\n");
        goto skip_jit_test;
    }

    // Register fibonacci function
    // For demonstration, we start with a JIT-compiled O0 version
    micro_jit_ctx_t initial_ctx;
    micro_jit_init(&initial_ctx, NULL);
    void* fib_o0 = micro_jit_compile_fibonacci(&initial_ctx, 5);

    int fib_id = adaptive_jit_register_function(&ajit, "fibonacci", "demo", fib_o0);
    if (fib_id < 0) {
        serial_puts("[3] Function registration FAILED\n");
        goto cleanup_ajit;
    }
    serial_puts("[3] fibonacci registered with adaptive JIT\n\n");

    // Execute fibonacci multiple times to trigger recompilation
    serial_puts("=== HOT-PATH DETECTION TEST ===\n");
    serial_puts("Executing fibonacci 150 times to trigger O0->O1->O2 recompilation\n\n");

    for (int i = 0; i < 150; i++) {
        int result = adaptive_jit_execute(&ajit, fib_id);

        // Print status at key thresholds
        if (i == 0) {
            serial_puts("[Call 1] Initial execution at O0\n");
        } else if (i == 99) {
            serial_puts("[Call 100] Threshold reached - recompiling to O1...\n");
        } else if (i == 100) {
            function_profile_t* profile = adaptive_jit_get_profile(&ajit, fib_id);
            serial_puts("[Call 101] Now running at O");
            serial_putchar('0' + profile->opt_level);
            serial_puts("\n");
        } else if (i == 149) {
            function_profile_t* profile = adaptive_jit_get_profile(&ajit, fib_id);
            serial_puts("[Call 150] Final optimization level: O");
            serial_putchar('0' + profile->opt_level);
            serial_puts("\n\n");
        }

        // Verify result
        if (result != 5 && i == 0) {
            serial_puts("ERROR: fibonacci(5) returned incorrect result!\n");
            break;
        }
    }

    // Print final profiling statistics
    function_profile_t* final_profile = adaptive_jit_get_profile(&ajit, fib_id);
    serial_puts("=== PROFILING STATISTICS ===\n");
    serial_puts("Function: fibonacci(5)\n");
    serial_puts("Total calls: ");
    // Simple integer printing (hundreds, tens, ones)
    int calls = (int)final_profile->call_count;
    serial_putchar('0' + (calls / 100));
    serial_putchar('0' + ((calls / 10) % 10));
    serial_putchar('0' + (calls % 10));
    serial_puts("\n");
    serial_puts("Final optimization level: O");
    serial_putchar('0' + final_profile->opt_level);
    serial_puts("\n");
    serial_puts("Recompilations triggered: ");
    if (final_profile->opt_level == OPT_LEVEL_O1) serial_puts("1 (O0->O1)\n");
    else if (final_profile->opt_level == OPT_LEVEL_O2) serial_puts("2 (O0->O1->O2)\n");
    else if (final_profile->opt_level == OPT_LEVEL_O3) serial_puts("3 (O0->O1->O2->O3)\n");
    else serial_puts("0\n");

    serial_puts("\n=== ADAPTIVE JIT TEST COMPLETE ===\n\n");

cleanup_ajit:
    adaptive_jit_shutdown(&ajit);
    micro_jit_destroy(&initial_ctx);

skip_jit_test:

    // Initialize C++ runtime (for new/delete, global constructors, etc.)
    cxx_runtime_init();

    // Test C++ runtime functionality
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    test_cxx_runtime();
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Wait for user to review C++ test results
#ifdef INTERACTIVE_MODE
    terminal_writestring("\nPress any key to continue to JIT allocator tests...\n");
    wait_key();
#endif
    terminal_writestring("\n");

    // Test JIT allocator functionality
    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    test_jit_allocator();
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Wait for user to review JIT allocator test results
#ifdef INTERACTIVE_MODE
    terminal_writestring("\nPress any key to continue to FAT16 tests...\n");
    wait_key();
#endif
    terminal_writestring("\n");

    // Test FAT16 filesystem (will fail gracefully if no disk attached)
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    test_fat16_filesystem();
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Wait for user to review FAT16 test results
#ifdef INTERACTIVE_MODE
    terminal_writestring("\nPress any key to continue to module tests...\n");
    wait_key();
#endif
    terminal_writestring("\n");

    // VERY VISIBLE boot message - WHITE ON RED
    terminal_setcolor(VGA_WHITE, VGA_RED);
    terminal_writestring("================================================================================\n");
    terminal_writestring("                     [DEBUG] KERNEL BOOT TEST                                   \n");
    terminal_writestring("================================================================================\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_writestring("\n");

    // Test: Can we even reach here?
    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("Step 1: terminal_initialize() OK\n");
    terminal_writestring("Step 2: VGA write working\n");
    terminal_writestring("Step 3: About to call print_banner()\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Print banner
    print_banner();

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("Step 4: print_banner() OK\n");
    terminal_writestring("Step 5: About to call print_system_info()\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // System info
    print_system_info();

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("Step 6: print_system_info() OK\n");
    terminal_writestring("Step 7: About to call check_cpu_features()\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // CPU features
    check_cpu_features();

    // Success message
    terminal_setcolor(VGA_GREEN, VGA_BLACK);
    terminal_writestring("Kernel initialized successfully!\n\n");

    // ========================================================================
    // MODULE SYSTEM TEST
    // ========================================================================

    terminal_setcolor(VGA_CYAN, VGA_BLACK);
    terminal_writestring("========================================\n");
    terminal_writestring("  DYNAMIC MODULE SYSTEM - LLVM AOT\n");
    terminal_writestring("========================================\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("[DEBUG] Initializing module manager...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Initialize module manager
    module_manager_t module_mgr;
    module_init(&module_mgr);

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("[DEBUG] Module manager initialized OK\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[INIT] Loading embedded modules...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Load all embedded modules
    int loaded = load_embedded_modules(&module_mgr);

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("[DEBUG] Modules loaded OK\n");

    // Load optimized modules from cache (if available)
    terminal_setcolor(VGA_LIGHT_MAGENTA, VGA_BLACK);
    terminal_writestring("[CACHE] Loading optimized modules...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    cache_load_modules(&module_mgr);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Try loading modules from disk (FAT16)
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writestring("[DISK] Attempting to load modules from FAT16...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    fat16_fs_t fs;
    if (fat16_init(&fs, 1, 0) == 0) {
        terminal_setcolor(VGA_GREEN, VGA_BLACK);
        terminal_writestring("  FAT16 initialized successfully on drive 1\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

        int disk_loaded = disk_load_all_modules(&module_mgr, &fs);
        if (disk_loaded > 0) {
            terminal_setcolor(VGA_GREEN, VGA_BLACK);
            terminal_writestring("  Loaded ");
            print_int(disk_loaded);
            terminal_writestring(" modules from disk\n");
            terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
        } else {
            terminal_setcolor(VGA_YELLOW, VGA_BLACK);
            terminal_writestring("  No .MOD files found on disk\n");
            terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
        }
    } else {
        terminal_setcolor(VGA_YELLOW, VGA_BLACK);
        terminal_writestring("  FAT16 init failed, using embedded modules only\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    }

    terminal_setcolor(VGA_GREEN, VGA_BLACK);
    terminal_writestring("\n[OK] Loaded ");
    print_int(loaded);
    terminal_writestring(" modules\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // ========================================================================
    // TEST 1: Simple Sum (warm-up)
    // ========================================================================

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("[DEBUG] Starting TEST 1...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 1] Simple Sum Module\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("[DEBUG] About to execute 'sum' module...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    int result = module_execute(&module_mgr, "sum");

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("[DEBUG] Module executed OK\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  Result: ");
    print_int(result);
    terminal_writestring(" (expected: 5050)\n");

    terminal_writestring("  DEBUG: Returned value is ");
    print_int(result);
    terminal_writestring("\n");

    if (result == 5050) {
        vga_print_color("  [OK] Test passed!\n", VGA_GREEN, VGA_BLACK);
    } else {
        vga_print_color("  [FAIL] Test failed!\n", VGA_RED, VGA_BLACK);
    }

    pause_for_key();

    // ========================================================================
    // TEST 2: Fibonacci (optimization test)
    // ========================================================================

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 2] Fibonacci Module\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    result = module_execute(&module_mgr, "fibonacci");

    terminal_writestring("  Result: ");
    print_int(result);
    terminal_writestring(" (expected: 6765)\n");

    terminal_writestring("  DEBUG: Returned value is ");
    print_int(result);
    terminal_writestring("\n");

    if (result == 6765) {
        vga_print_color("  [OK] Test passed!\n", VGA_GREEN, VGA_BLACK);
    } else {
        vga_print_color("  [FAIL] Test failed!\n", VGA_RED, VGA_BLACK);
    }

    pause_for_key();

    // ========================================================================
    // TEST 3: Compute Intensive (profiling test)
    // ========================================================================

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 3] Compute Intensive Module\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  Running first iteration...\n");
    result = module_execute(&module_mgr, "compute");

    terminal_writestring("  First result: ");
    print_int(result);
    terminal_writestring(" (should be consistent)\n");

    terminal_writestring("  Running 9 more iterations for profiling...\n");

    for (int i = 0; i < 9; i++) {
        module_execute(&module_mgr, "compute");
    }

    vga_print_color("  [OK] 10 iterations completed\n", VGA_GREEN, VGA_BLACK);

    pause_for_key();

    // ========================================================================
    // TEST 4: Prime Counter (advanced algorithm)
    // ========================================================================

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 4] Prime Counter Module\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  Counting primes < 1000...\n");

    result = module_execute(&module_mgr, "primes");

    terminal_writestring("  Result: ");
    print_int(result);
    terminal_writestring(" primes found (expected: 168)\n");

    terminal_writestring("  DEBUG: Returned value is ");
    print_int(result);
    terminal_writestring("\n");

    if (result == 168) {
        vga_print_color("  [OK] Test passed!\n", VGA_GREEN, VGA_BLACK);
    } else {
        vga_print_color("  [FAIL] Test failed!\n", VGA_RED, VGA_BLACK);
    }

    pause_for_key();

    // ========================================================================
    // TEST 5: Matrix Multiplication (Performance benchmark)
    // ========================================================================
    // 16x16 matrices using .data section (fully embedded in .mod file)

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 5] Matrix Multiplication (16x16)\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  Running matrix multiplication benchmark...\n");

    result = module_execute(&module_mgr, "matrix_mul");

    terminal_writestring("  Result (checksum): ");
    print_int(result);
    terminal_writestring("\n");

    if (result == -1) {
        vga_print_color("  [ERROR] Memory allocation failed!\n", VGA_RED, VGA_BLACK);
    } else {
        terminal_writestring("  Running 4 more iterations for profiling...\n");

        for (int i = 0; i < 4; i++) {
            module_execute(&module_mgr, "matrix_mul");
        }

        vga_print_color("  [OK] 5 iterations completed\n", VGA_GREEN, VGA_BLACK);
    }

    pause_for_key();

    // ========================================================================
    // TEST 6: FFT 1D
    // ========================================================================
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 6] FFT 1D (32 samples)\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    result = module_execute(&module_mgr, "fft_1d");
    terminal_writestring("  Result (checksum): ");
    print_int(result);
    terminal_writestring("\n");

    pause_for_key();

    // ========================================================================
    // TEST 7: SHA256
    // ========================================================================
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 7] SHA256 (1KB)\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    result = module_execute(&module_mgr, "sha256");
    terminal_writestring("  Result (checksum): ");
    print_int(result);
    terminal_writestring("\n");

    pause_for_key();

    // ========================================================================
    // TEST 8: Quicksort
    // ========================================================================
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 8] Quicksort (128 elements, 5 iterations)\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    result = module_execute(&module_mgr, "quicksort");
    terminal_writestring("  Result (checksum): ");
    print_int(result);
    terminal_writestring("\n");

    pause_for_key();

    // ========================================================================
    // TEST 9: String Operations
    // ========================================================================
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 9] String Operations (100 iterations)\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    result = module_execute(&module_mgr, "strops");
    terminal_writestring("  Result (checksum): ");
    print_int(result);
    terminal_writestring("\n");

    pause_for_key();

    // ========================================================================
    // PROFILING STATISTICS (one module at a time)
    // ========================================================================

    terminal_setcolor(VGA_CYAN, VGA_BLACK);
    terminal_writestring("\n========================================\n");
    terminal_writestring("    PROFILING STATISTICS\n");
    terminal_writestring("========================================\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("Total modules: ");
    print_int(module_mgr.num_modules);
    terminal_writestring("\n");
    terminal_writestring("Total calls: ");
    print_int((int)module_mgr.total_calls);
    terminal_writestring("\n");

    pause_for_key();

    // Show each module stats individually
    module_print_stats(&module_mgr, "sum");
    pause_for_key();

    module_print_stats(&module_mgr, "fibonacci");
    pause_for_key();

    module_print_stats(&module_mgr, "compute");
    pause_for_key();

    module_print_stats(&module_mgr, "primes");
    pause_for_key();

    module_print_stats(&module_mgr, "matrix_mul");
    pause_for_key();

    // ========================================================================
    // PROFILING DATA EXPORT
    // ========================================================================

    terminal_setcolor(VGA_CYAN, VGA_BLACK);
    terminal_writestring("\n========================================\n");
    terminal_writestring("   PROFILING DATA EXPORT\n");
    terminal_writestring("========================================\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("Initializing serial port (COM1)...\n");
    if (serial_init() == 0) {
        terminal_setcolor(VGA_GREEN, VGA_BLACK);
        terminal_writestring("[OK] Serial port initialized (115200 baud)\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
        terminal_writestring("Exporting profiling data to serial port...\n");
        terminal_writestring("(VGA output paused during export to avoid mixing)\n\n");

        // Export profiling data (SERIAL OUTPUT ONLY - no VGA during export)
        profiling_trigger_export(&module_mgr);

        // Resume VGA output
        terminal_setcolor(VGA_GREEN, VGA_BLACK);
        terminal_writestring("\n[OK] Profiling data exported to serial port\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    } else {
        terminal_setcolor(VGA_RED, VGA_BLACK);
        terminal_writestring("[FAIL] Serial port initialization failed\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    }

    // ========================================================================
    // END-TO-END JIT DEMO
    // ========================================================================

    //serial_puts("\n");
    //jit_demo_disk_to_jit();

    // ========================================================================
    // ELF LOADER TEST
    // ========================================================================

    test_elf_loader();

    // ========================================================================
    // LLVM ADAPTIVE OPTIMIZATION DEMO
    // ========================================================================

    test_llvm_modules();

    // ========================================================================
    // LLVM PGO PERFORMANCE TEST SUITE
    // ========================================================================

    test_llvm_pgo_suite();

    // ========================================================================
    // FINAL MESSAGE
    // ========================================================================

    terminal_setcolor(VGA_GREEN, VGA_BLACK);
    terminal_writestring("\n=== ALL MODULE TESTS COMPLETED ===\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("System ready. CPU halted.\n");

    // Infinite loop
    while (1) {
        asm volatile("hlt");
    }
}
