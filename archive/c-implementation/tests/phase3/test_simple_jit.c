// Simple JIT test - return 42
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>

int main() {
    printf("=== Simple JIT Test: return 42 ===\n");

    // Allocate executable memory
    uint8_t* code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        printf("FAILED: mmap\n");
        return 1;
    }

    // Generate code: mov eax, 42; ret
    code[0] = 0xB8;  // mov eax, imm32
    code[1] = 42;
    code[2] = 0;
    code[3] = 0;
    code[4] = 0;
    code[5] = 0xC3;  // ret

    printf("Code generated:\n");
    for (int i = 0; i < 6; i++) {
        printf("  %02X", code[i]);
    }
    printf("\n\n");

    // Execute
    typedef int (*func_t)(void);
    func_t func = (func_t)code;
    int result = func();

    printf("Result: %d\n", result);
    printf("Expected: 42\n");
    printf("%s\n", result == 42 ? "[OK]" : "[FAILED]");

    munmap(code, 4096);
    return result == 42 ? 0 : 1;
}
