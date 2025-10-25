// ============================================================================
// Function Profiler Test
// ============================================================================
// Demonstrates per-function profiling and JIT trigger detection
// ============================================================================

#include "function_profiler.h"
#include "vga.h"

// External functions
extern void terminal_writestring(const char* str);
extern void terminal_setcolor(enum vga_color fg, enum vga_color bg);

// Simple test functions to profile
static int test_fibonacci(int n) {
    if (n <= 1) return n;
    return test_fibonacci(n - 1) + test_fibonacci(n - 2);
}

static int test_sum(int n) {
    int sum = 0;
    for (int i = 1; i <= n; i++) {
        sum += i;
    }
    return sum;
}

static int test_factorial(int n) {
    if (n <= 1) return 1;
    return n * test_factorial(n - 1);
}

// Test function profiler
void test_function_profiler(void) {
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writestring("\n=== FUNCTION PROFILER TEST ===\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Initialize profiler with JIT enabled
    function_profiler_t profiler;
    function_profiler_init(&profiler, true);

    // Register test functions
    int fib_id = function_profiler_register(&profiler, "test_fibonacci", "test", (void*)test_fibonacci);
    int sum_id = function_profiler_register(&profiler, "test_sum", "test", (void*)test_sum);
    int fact_id = function_profiler_register(&profiler, "test_factorial", "test", (void*)test_factorial);

    terminal_writestring("Registered 3 functions for profiling\n\n");

    // Execute functions with profiling
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("[TEST 1] Calling test_fibonacci(10) x 50 times\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    for (int i = 0; i < 50; i++) {
        uint64_t start, end;
        __asm__ __volatile__("rdtsc" : "=A"(start));
        int result = test_fibonacci(10);
        __asm__ __volatile__("rdtsc" : "=A"(end));
        function_profiler_record(&profiler, fib_id, end - start);
        (void)result;  // Suppress unused warning
    }

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\n[TEST 2] Calling test_sum(1000) x 150 times\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    for (int i = 0; i < 150; i++) {
        uint64_t start, end;
        __asm__ __volatile__("rdtsc" : "=A"(start));
        int result = test_sum(1000);
        __asm__ __volatile__("rdtsc" : "=A"(end));
        function_profiler_record(&profiler, sum_id, end - start);
        (void)result;
    }

    // Check if sum should be recompiled (should trigger at 100 calls)
    if (function_profiler_needs_recompile(&profiler, sum_id)) {
        terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
        terminal_writestring("✓ test_sum reached JIT threshold! Would recompile to O1\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
        function_profiler_mark_recompiled(&profiler, sum_id, OPT_LEVEL_O1);
    }

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\n[TEST 3] Calling test_factorial(10) x 20 times\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    for (int i = 0; i < 20; i++) {
        uint64_t start, end;
        __asm__ __volatile__("rdtsc" : "=A"(start));
        int result = test_factorial(10);
        __asm__ __volatile__("rdtsc" : "=A"(end));
        function_profiler_record(&profiler, fact_id, end - start);
        (void)result;
    }

    // Get hot functions
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writestring("\n[HOT FUNCTION DETECTION]\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    int hot_funcs[10];
    int hot_count = function_profiler_get_hot_functions(&profiler, hot_funcs, 10);

    terminal_writestring("Top hot functions (by total cycles):\n");
    for (int i = 0; i < hot_count && i < 3; i++) {
        int func_id = hot_funcs[i];
        function_profile_t* func = &profiler.functions[func_id];
        terminal_writestring("  ");
        terminal_writestring(func->name);
        terminal_writestring(" [HOT]\n");
    }

    // Print full statistics
    function_profiler_print_stats(&profiler);

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("\n✓ Function profiler test complete!\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}
