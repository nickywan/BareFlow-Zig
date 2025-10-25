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
    // Initialize I/O
    vga_init();
    serial_init();

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

    // ========================================================================
    // Summary
    // ========================================================================
    vga_setcolor(0x0A, 0x00);
    vga_writestring("\n===========================================\n");
    vga_writestring("  ALL PROFILING STATS:\n");
    vga_writestring("===========================================\n");
    vga_setcolor(0x0F, 0x00);

    jit_print_all_stats(&profiler);

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
