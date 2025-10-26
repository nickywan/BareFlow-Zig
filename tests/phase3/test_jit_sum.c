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
    printf("Testing sum(1..100)...\n");
    
    micro_jit_ctx_t ctx;
    if (micro_jit_init(&ctx, NULL) != 0) {
        printf("Init failed\n");
        return 1;
    }
    
    typedef int (*func_t)(void);
    func_t sum = (func_t)micro_jit_compile_sum(&ctx, 100);
    if (!sum) {
        printf("Compile failed\n");
        return 1;
    }
    
    int result = sum();
    printf("sum(1..100) = %d\n", result);
    printf("Expected: 5050\n");
    printf("%s\n", result == 5050 ? "[OK]" : "[FAILED]");
    
    micro_jit_destroy(&ctx);
    return result == 5050 ? 0 : 1;
}
