// Module: prime counter benchmark

#include <stdint.h>

#define MODULE_MAGIC 0x4D4F4442

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

static int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int primes_count(void) {
    int count = 0;
    for (int i = 0; i < 1000; ++i) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "primes",
    .entry_point = (void*)primes_count,
    .code_size = 0,
    .version = 1
};
