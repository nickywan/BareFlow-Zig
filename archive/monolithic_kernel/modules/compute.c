// Module: Compute intensive task
// Tests optimization and profiling

#include <stdint.h>

#define MODULE_MAGIC 0x4D4F4442

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

// Compute-intensive function (will benefit from LLVM optimizations)
int compute_intensive(void) {
    int result = 0;

    // Nested loops - LLVM should optimize this
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            result += (i * j) % 1000;
        }
    }

    return result;
}

__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "compute",
    .entry_point = (void*)compute_intensive,
    .code_size = 0,
    .version = 1
};
