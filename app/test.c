// app/test.c - Test simple de stdlib en LLVM IR

#include <stddef.h>

// Forward declarations de stdlib
extern void* malloc(size_t size);
extern void free(void* ptr);
extern void* memset(void* s, int c, size_t n);
extern void* memcpy(void* dest, const void* src, size_t n);
extern size_t strlen(const char* s);
extern char* strcpy(char* dest, const char* src);
extern int strcmp(const char* s1, const char* s2);

// Cette fonction sera appelée depuis le kernel
int test_main() {
    // Test 1: malloc + memset
    char* buffer1 = (char*)malloc(100);
    if (!buffer1)
        return -1;
    
    memset(buffer1, 'A', 99);
    buffer1[99] = '\0';
    
    // Test 2: strlen
    size_t len = strlen(buffer1);
    if (len != 99)
        return -2;
    
    // Test 3: malloc + strcpy
    char* buffer2 = (char*)malloc(50);
    if (!buffer2)
        return -3;
    
    const char* test_string = "Hello Fluid!";
    strcpy(buffer2, test_string);
    
    // Test 4: strcmp
    if (strcmp(buffer2, "Hello Fluid!") != 0)
        return -4;
    
    // Test 5: memcpy
    char* buffer3 = (char*)malloc(50);
    if (!buffer3)
        return -5;
    
    memcpy(buffer3, buffer2, strlen(buffer2) + 1);
    
    if (strcmp(buffer3, "Hello Fluid!") != 0)
        return -6;
    
    // Test 6: Loop (sera optimisé par JIT)
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    
    // sum devrait être 499500
    if (sum != 499500)
        return -7;
    
    // Tous les tests passés !
    return 0;
}

// Fonction avec hot loop (pour profiling/JIT)
int compute_intensive() {
    int result = 0;
    
    // Cette boucle sera profilée et JIT-optimisée
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 100; j++) {
            result += i * j;
        }
    }
    
    return result;
}

// Fonction simple (ne sera probablement pas JIT-optimisée)
int simple_add(int a, int b) {
    return a + b;
}
