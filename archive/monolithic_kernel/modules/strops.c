// ============================================================================
// String Operations Benchmark - Tests memory access patterns and loops
// ============================================================================
#include "../kernel/module_loader.h"

static const module_header_t __attribute__((section(".module_header"))) module_header = {
    .magic = MODULE_MAGIC,
    .name = "strops",
    .entry_point = (void*)0,
    .code_size = 0,
    .version = 1
};

// Inline string length
static int my_strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

// Inline string copy
static void my_strcpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

// Inline string reverse
static void my_strrev(char* str, int len) {
    int i = 0;
    int j = len - 1;
    while (i < j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

// String comparison
static int my_strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

__attribute__((section(".text.entry")))
int strops_benchmark(void) {
    // Test strings in .data section
    static char buffer1[64] = "The quick brown fox jumps over the lazy dog";
    static char buffer2[64] = {1};
    static char buffer3[64] = {1};

    int checksum = 0;

    // Perform string operations 100 times
    for (int iter = 0; iter < 100; iter++) {
        // Test strlen
        int len1 = my_strlen(buffer1);
        checksum += len1;

        // Test strcpy
        my_strcpy(buffer2, buffer1);
        checksum += buffer2[5];

        // Test strrev
        my_strcpy(buffer3, buffer1);
        my_strrev(buffer3, len1);
        checksum += buffer3[0];

        // Test strcmp
        int cmp = my_strcmp(buffer1, buffer2);
        checksum += cmp;

        // Modify buffer slightly for next iteration
        if (buffer1[0] < 'z') buffer1[0]++;
        else buffer1[0] = 'A';
    }

    return checksum;
}
