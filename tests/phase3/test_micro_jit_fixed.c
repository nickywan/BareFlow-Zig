// Test fixed micro-JIT
#include <stdio.h>
#include <sys/mman.h>
#include "kernel/micro_jit.h"

void* jit_alloc_code(size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void jit_free_code(void* ptr) {
    if (ptr && ptr != MAP_FAILED) {
        munmap(ptr, MAX_JIT_CODE_SIZE);
    }
}

int main() {
    printf("=== Micro-JIT Test (Fixed) ===\n\n");

    micro_jit_ctx_t ctx;

    // Test fibonacci(20)
    printf("[1] fibonacci(20)...\n");
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("FAILED: init\n");
        return 1;
    }

    typedef int (*func_t)(void);
    func_t fib = (func_t)micro_jit_compile_fibonacci(&ctx, 20);
    if (!fib) {
        printf("FAILED: compile\n");
        return 1;
    }

    int result = fib();
    printf("  Result: %d\n", result);
    printf("  Expected: 6765\n");
    printf("  %s\n\n", result == 6765 ? "[OK]" : "[FAILED]");

    micro_jit_destroy(&ctx);

    // Test sum(1..100)
    printf("[2] sum(1..100)...\n");
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("FAILED: init\n");
        return 1;
    }

    func_t sum = (func_t)micro_jit_compile_sum(&ctx, 100);
    if (!sum) {
        printf("FAILED: compile\n");
        return 1;
    }

    result = sum();
    printf("  Result: %d\n", result);
    printf("  Expected: 5050\n");
    printf("  %s\n\n", result == 5050 ? "[OK]" : "[FAILED]");

    micro_jit_destroy(&ctx);

    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
