// Debug fibonacci x86 generation
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>

int main() {
    printf("=== Fibonacci x86 Debug ===\n\n");

    uint8_t* code = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) {
        printf("FAILED: mmap\n");
        return 1;
    }

    // Simplified fibonacci(5):
    // eax = a, ecx = b, edx = i
    int pos = 0;

    // mov eax, 0
    code[pos++] = 0xB8;
    code[pos++] = 0; code[pos++] = 0; code[pos++] = 0; code[pos++] = 0;

    // mov ecx, 1
    code[pos++] = 0xB9;
    code[pos++] = 1; code[pos++] = 0; code[pos++] = 0; code[pos++] = 0;

    // mov edx, 0
    code[pos++] = 0xBA;
    code[pos++] = 0; code[pos++] = 0; code[pos++] = 0; code[pos++] = 0;

    // loop: (offset 15)
    int loop_start = pos;

    // cmp edx, 5
    code[pos++] = 0x81;
    code[pos++] = 0xFA;
    code[pos++] = 5; code[pos++] = 0; code[pos++] = 0; code[pos++] = 0;

    // jge end (we'll patch this)
    int jge_pos = pos;
    code[pos++] = 0x0F;
    code[pos++] = 0x8D;
    code[pos++] = 0; code[pos++] = 0; code[pos++] = 0; code[pos++] = 0;

    // mov ebx, eax  (temp = a)
    code[pos++] = 0x89;
    code[pos++] = 0xC3;

    // add ebx, ecx  (temp += b)
    code[pos++] = 0x01;
    code[pos++] = 0xCB;

    // mov eax, ecx  (a = b)
    code[pos++] = 0x89;
    code[pos++] = 0xC8;

    // mov ecx, ebx  (b = temp)
    code[pos++] = 0x89;
    code[pos++] = 0xD9;

    // inc edx
    code[pos++] = 0xFF;
    code[pos++] = 0xC2;

    // jmp loop
    int jmp_offset = loop_start - (pos + 5);
    code[pos++] = 0xE9;
    code[pos++] = jmp_offset & 0xFF;
    code[pos++] = (jmp_offset >> 8) & 0xFF;
    code[pos++] = (jmp_offset >> 16) & 0xFF;
    code[pos++] = (jmp_offset >> 24) & 0xFF;

    // end: patch jge
    int end_pos = pos;
    int jge_offset = end_pos - (jge_pos + 6);
    code[jge_pos + 2] = jge_offset & 0xFF;
    code[jge_pos + 3] = (jge_offset >> 8) & 0xFF;
    code[jge_pos + 4] = (jge_offset >> 16) & 0xFF;
    code[jge_pos + 5] = (jge_offset >> 24) & 0xFF;

    // ret
    code[pos++] = 0xC3;

    printf("Generated %d bytes of code\n", pos);
    printf("Code dump:\n");
    for (int i = 0; i < pos; i++) {
        printf("%02X ", code[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\n");

    // Execute
    typedef int (*func_t)(void);
    func_t func = (func_t)code;
    int result = func();

    printf("fibonacci(5) = %d\n", result);
    printf("Expected: 5 (0, 1, 1, 2, 3, 5)\n");
    printf("%s\n", result == 5 ? "[OK]" : "[FAILED]");

    munmap(code, 4096);
    return result == 5 ? 0 : 1;
}
