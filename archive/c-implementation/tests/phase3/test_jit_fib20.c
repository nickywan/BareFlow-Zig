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
    printf("Testing fibonacci(20)...\n");
    
    micro_jit_ctx_t ctx;
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("Init failed\n");
        return 1;
    }
    
    typedef int (*func_t)(void);
    func_t fib = (func_t)micro_jit_compile_fibonacci(&ctx, 20);
    if (!fib) {
        printf("Compile failed\n");
        return 1;
    }
    
    int result = fib();
    printf("fibonacci(20) = %d\n", result);
    printf("Expected: 6765\n");
    printf("%s\n", result == 6765 ? "[OK]" : "[FAILED]");
    
    micro_jit_destroy(&ctx);
    return result == 6765 ? 0 : 1;
}
