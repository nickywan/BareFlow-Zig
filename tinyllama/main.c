/**
 * TinyLlama Unikernel - Self-Profiling Demonstration
 *
 * Demonstrates the kernel_lib.a runtime library with JIT profiling.
 * This is a minimal self-optimizing program that profiles its own execution.
 */

#include "../kernel_lib/runtime.h"
#include "../kernel_lib/jit_runtime.h"

// ============================================================================
// DEMO FUNCTIONS
// ============================================================================

/**
 * Fibonacci calculation (recursive)
 */
static int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

/**
 * Simple sum from 1 to N
 */
static int sum_to_n(int n) {
    int sum = 0;
    for (int i = 1; i <= n; i++) {
        sum += i;
    }
    return sum;
}

/**
 * Dummy function for overhead benchmark
 */
static inline int dummy_function(int x) {
    return x + 1;
}

/**
 * Prime counting function
 */
static int count_primes(int max) {
    int count = 0;
    for (int num = 2; num <= max; num++) {
        int is_prime = 1;
        for (int i = 2; i * i <= num; i++) {
            if (num % i == 0) {
                is_prime = 0;
                break;
            }
        }
        if (is_prime) count++;
    }
    return count;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

void main(void) {
    // Start timing
    uint64_t start = cpu_rdtsc();

    // Initialize I/O
    vga_init();
    serial_init();

    uint64_t init_done = cpu_rdtsc();

    vga_setcolor(0x0A, 0x00);  // Green on black
    vga_writestring("===========================================\n");
    vga_writestring("  TinyLlama Unikernel v0.1\n");
    vga_writestring("  Self-Profiling Demo\n");
    vga_writestring("===========================================\n\n");

    serial_puts("\n[tinyllama] TinyLlama Unikernel v0.1 - Self-Profiling Demo\n");
    serial_puts("[tinyllama] Initializing JIT profiler...\n");

    // Initialize JIT profiler
    jit_profile_t profiler;
    jit_profile_init(&profiler);

    vga_setcolor(0x0F, 0x00);  // White on black

    // ========================================================================
    // Benchmark: Function Call Overhead
    // ========================================================================
    serial_puts("\n[BENCH] Measuring function call overhead...\n");

    volatile int result = 0;  // Prevent optimization
    uint64_t bench_start = cpu_rdtsc();

    for (int i = 0; i < 10000; i++) {
        result = dummy_function(i);
    }

    uint64_t bench_end = cpu_rdtsc();
    uint64_t bench_total = bench_end - bench_start;
    uint64_t bench_per_call = bench_total / 10000;

    serial_puts("[BENCH] 10000 calls: ");
    serial_put_uint64(bench_total);
    serial_puts(" cycles (");
    serial_put_uint64(bench_per_call);
    serial_puts(" cycles/call)\n\n");

    // ========================================================================
    // Test 1: Fibonacci (10 iterations)
    // ========================================================================
    vga_writestring("Test 1: Fibonacci(10) - 10 iterations\n");
    serial_puts("[tinyllama] Running Fibonacci test...\n");

    for (int i = 0; i < 10; i++) {
        jit_profile_begin(&profiler, "fibonacci");
        int result = fibonacci(10);
        jit_profile_end(&profiler, "fibonacci");

        if (i == 0) {
            vga_writestring("  First result: ");
            char buf[16];
            int idx = 0;
            int n = result;
            do {
                buf[idx++] = '0' + (n % 10);
                n /= 10;
            } while (n > 0);
            for (int j = idx - 1; j >= 0; j--) {
                vga_putchar(buf[j]);
            }
            vga_writestring("\n");
        }
    }

    vga_setcolor(0x0E, 0x00);  // Yellow on black
    vga_writestring("  ");
    jit_print_stats(&profiler, "fibonacci");
    vga_writestring("\n");

    uint64_t test1_done = cpu_rdtsc();

    // ========================================================================
    // Test 2: Sum to N (100 iterations)
    // ========================================================================
    vga_setcolor(0x0F, 0x00);
    vga_writestring("Test 2: Sum(1..1000) - 100 iterations\n");
    serial_puts("[tinyllama] Running Sum test...\n");

    for (int i = 0; i < 100; i++) {
        jit_profile_begin(&profiler, "sum_to_n");
        int result = sum_to_n(1000);
        jit_profile_end(&profiler, "sum_to_n");

        if (i == 0) {
            vga_writestring("  First result: ");
            char buf[16];
            int idx = 0;
            int n = result;
            do {
                buf[idx++] = '0' + (n % 10);
                n /= 10;
            } while (n > 0);
            for (int j = idx - 1; j >= 0; j--) {
                vga_putchar(buf[j]);
            }
            vga_writestring("\n");
        }
    }

    vga_setcolor(0x0E, 0x00);
    vga_writestring("  ");
    jit_print_stats(&profiler, "sum_to_n");
    vga_writestring("\n");

    uint64_t test2_done = cpu_rdtsc();

    // ========================================================================
    // Test 3: Prime Counting (5 iterations)
    // ========================================================================
    vga_setcolor(0x0F, 0x00);
    vga_writestring("Test 3: Count primes up to 100 - 5 iterations\n");
    serial_puts("[tinyllama] Running Prime counting test...\n");

    for (int i = 0; i < 5; i++) {
        jit_profile_begin(&profiler, "count_primes");
        int result = count_primes(100);
        jit_profile_end(&profiler, "count_primes");

        if (i == 0) {
            vga_writestring("  Primes found: ");
            char buf[16];
            int idx = 0;
            int n = result;
            do {
                buf[idx++] = '0' + (n % 10);
                n /= 10;
            } while (n > 0);
            for (int j = idx - 1; j >= 0; j--) {
                vga_putchar(buf[j]);
            }
            vga_writestring("\n");
        }
    }

    vga_setcolor(0x0E, 0x00);
    vga_writestring("  ");
    jit_print_stats(&profiler, "count_primes");
    vga_writestring("\n");

    uint64_t test3_done = cpu_rdtsc();

    // ========================================================================
    // Summary
    // ========================================================================
    vga_setcolor(0x0A, 0x00);
    vga_writestring("\n===========================================\n");
    vga_writestring("  ALL PROFILING STATS:\n");
    vga_writestring("===========================================\n");
    vga_setcolor(0x0F, 0x00);

    jit_print_all_stats(&profiler);

    uint64_t end = cpu_rdtsc();

    // ========================================================================
    // Performance Timing Report
    // ========================================================================
    serial_puts("\n=== PERFORMANCE TIMING ===\n");
    serial_puts("[TIMING] Initialization:  ");
    serial_put_uint64(init_done - start);
    serial_puts(" cycles\n");

    serial_puts("[TIMING] Test 1 (Fib):    ");
    serial_put_uint64(test1_done - init_done);
    serial_puts(" cycles\n");

    serial_puts("[TIMING] Test 2 (Sum):    ");
    serial_put_uint64(test2_done - test1_done);
    serial_puts(" cycles\n");

    serial_puts("[TIMING] Test 3 (Primes): ");
    serial_put_uint64(test3_done - test2_done);
    serial_puts(" cycles\n");

    serial_puts("[TIMING] Total execution: ");
    serial_put_uint64(end - start);
    serial_puts(" cycles\n");
    serial_puts("==========================\n\n");

    vga_setcolor(0x0A, 0x00);
    vga_writestring("\n===========================================\n");
    vga_writestring("  Self-Profiling Demo Complete!\n");
    vga_writestring("  System halted.\n");
    vga_writestring("===========================================\n");

    serial_puts("[tinyllama] Demo complete. System halted.\n");

    // Infinite loop (never return from main)
    while (1) {
        __asm__ volatile("hlt");
    }
}
