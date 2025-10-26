// Test des modules en userspace (sans boot)
// Simule l'environnement bare-metal

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Include module system headers
#include "kernel/module_loader.h"
#include "kernel/embedded_modules.h"

// Mock VGA functions for testing
void terminal_writestring(const char* str) {
    printf("%s", str);
}

void terminal_putchar(char c) {
    putchar(c);
}

void terminal_setcolor(uint8_t fg, uint8_t bg) {
    (void)fg;
    (void)bg;
}

// Module loader implementation (simplified for userspace)
#include "kernel/module_loader.c"

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  MODULE SYSTEM TEST (USERSPACE)\n");
    printf("========================================\n\n");

    // Initialize module manager
    module_manager_t mgr;
    module_init(&mgr);

    printf("[INIT] Loading embedded modules...\n");

    // Load all embedded modules
    int loaded = load_embedded_modules(&mgr);

    printf("\n[OK] Loaded %d modules\n\n", loaded);

    // ========================================================================
    // TEST 1: Simple Sum
    // ========================================================================

    printf("[TEST 1] Simple Sum Module\n");

    int result = module_execute(&mgr, "sum");
    printf("  Result: %d (expected: 5050)\n", result);

    if (result == 5050) {
        printf("  \033[32m[OK] Test passed!\033[0m\n\n");
    } else {
        printf("  \033[31m[FAIL] Test failed!\033[0m\n\n");
    }

    // ========================================================================
    // TEST 2: Fibonacci
    // ========================================================================

    printf("[TEST 2] Fibonacci Module\n");

    result = module_execute(&mgr, "fibonacci");
    printf("  Result: %d (expected: 6765)\n", result);

    if (result == 6765) {
        printf("  \033[32m[OK] Test passed!\033[0m\n\n");
    } else {
        printf("  \033[31m[FAIL] Test failed!\033[0m\n\n");
    }

    // ========================================================================
    // TEST 3: Compute Intensive
    // ========================================================================

    printf("[TEST 3] Compute Intensive Module (profiling)\n");
    printf("  Running 10 iterations...\n");

    for (int i = 0; i < 10; i++) {
        module_execute(&mgr, "compute");
    }

    printf("  \033[32m[OK] 10 iterations completed\033[0m\n\n");

    // ========================================================================
    // TEST 4: Prime Counter
    // ========================================================================

    printf("[TEST 4] Prime Counter Module\n");
    printf("  Counting primes < 1000...\n");

    result = module_execute(&mgr, "primes");
    printf("  Result: %d primes found (expected: 168)\n", result);

    if (result == 168) {
        printf("  \033[32m[OK] Test passed!\033[0m\n\n");
    } else {
        printf("  \033[31m[FAIL] Test failed!\033[0m\n\n");
    }

    // ========================================================================
    // PROFILING STATISTICS
    // ========================================================================

    module_print_all_stats(&mgr);

    printf("\033[32m\n=== ALL MODULE TESTS COMPLETED ===\033[0m\n\n");

    return 0;
}
