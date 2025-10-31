// Module: Simple sum
// Basic test module

#include <stdint.h>

#define MODULE_MAGIC 0x4D4F4442

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

// Simple sum function
int simple_sum(void) {
    int sum = 0;

    for (int i = 1; i <= 100; i++) {
        sum += i;
    }

    return sum;  // Returns 5050
}

__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "sum",
    .entry_point = (void*)simple_sum,
    .code_size = 0,
    .version = 1
};
