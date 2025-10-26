// Module: Fibonacci calculator
// This will be compiled to native code and loaded dynamically

#include <stdint.h>

// Module header (will be filled by build system)
#define MODULE_MAGIC 0x4D4F4442

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

// The actual function (entry point)
int fibonacci_calc(void) {
    // Calculate 20th Fibonacci number
    int a = 0, b = 1;

    for (int i = 0; i < 20; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }

    return a;  // Returns 6765
}

// Module metadata (will be at the beginning of the module)
__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "fibonacci",
    .entry_point = (void*)fibonacci_calc,
    .code_size = 0,  // Will be filled by linker
    .version = 1
};
