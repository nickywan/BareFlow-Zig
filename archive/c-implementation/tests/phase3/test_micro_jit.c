// Test Micro-JIT in userspace
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "kernel/micro_jit.h"

// Userspace JIT allocator (mmap with execute permissions)
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
    printf("=== Micro-JIT Test ===\n\n");

    micro_jit_ctx_t ctx;

    // Test 1: Fibonacci
    printf("[1] Compiling fibonacci(20)...\n");
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("    FAILED: init\n");
        return 1;
    }

    typedef int (*func_t)(void);
    func_t fib = (func_t)micro_jit_compile_fibonacci(&ctx, 20);
    if (!fib) {
        printf("    FAILED: compile\n");
        return 1;
    }

    int fib_result = fib();
    printf("    fibonacci(20) = %d\n", fib_result);
    printf("    Expected: 6765\n");
    printf("    %s\n\n", fib_result == 6765 ? "[OK]" : "[FAILED]");

    micro_jit_destroy(&ctx);

    // Test 2: Sum
    printf("[2] Compiling sum(1..100)...\n");
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("    FAILED: init\n");
        return 1;
    }

    func_t sum = (func_t)micro_jit_compile_sum(&ctx, 100);
    if (!sum) {
        printf("    FAILED: compile\n");
        return 1;
    }

    int sum_result = sum();
    printf("    sum(1..100) = %d\n", sum_result);
    printf("    Expected: 5050\n");
    printf("    %s\n\n", sum_result == 5050 ? "[OK]" : "[FAILED]");

    micro_jit_destroy(&ctx);

    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
