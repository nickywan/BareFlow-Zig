#include <stdio.h>
#include <sys/mman.h>
#include "kernel/micro_jit.h"

void* jit_alloc_code(size_t size) {
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("Allocated code at %p\n", ptr);
    return ptr;
}

void jit_free_code(void* ptr) {
    if (ptr && ptr != MAP_FAILED) {
        munmap(ptr, MAX_JIT_CODE_SIZE);
    }
}

int main() {
    printf("=== Micro-JIT Diagnostic ===\n\n");
    
    micro_jit_ctx_t ctx;
    
    printf("[1] Testing init...\n");
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("FAILED: init\n");
        return 1;
    }
    printf("  code_buffer = %p\n", ctx.code_buffer);
    printf("  code_size = %zu\n", ctx.code_size);
    printf("  code_capacity = %zu\n", ctx.code_capacity);
    
    printf("\n[2] Compiling fibonacci(5)...\n");
    void* fib = micro_jit_compile_fibonacci(&ctx, 5);
    if (!fib) {
        printf("FAILED: compile returned NULL\n");
        return 1;
    }
    printf("  Function at %p\n", fib);
    printf("  Generated %zu bytes\n", ctx.code_size);
    
    printf("\n[3] Dumping code:\n  ");
    for (size_t i = 0; i < ctx.code_size; i++) {
        printf("%02X ", ctx.code_buffer[i]);
        if ((i + 1) % 16 == 0 && i + 1 < ctx.code_size) printf("\n  ");
    }
    printf("\n\n[4] Calling...\n");
    
    typedef int (*func_t)(void);
    func_t func = (func_t)fib;
    int result = func();
    
    printf("  Result: %d\n", result);
    printf("  Expected: 5\n");
    printf("  %s\n", result == 5 ? "[OK]" : "[FAILED]");
    
    micro_jit_destroy(&ctx);
    return 0;
}
