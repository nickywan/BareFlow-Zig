// ============================================================================
// Simple Test Module for ELF Loader
// ============================================================================
// This module will be compiled as a freestanding ELF32 binary and loaded
// by the ELF loader to test basic functionality.

int test_function(void) {
    return 42;
}

int add_numbers(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    int result = 0;
    for (int i = 0; i < y; i++) {
        result += x;
    }
    return result;
}
