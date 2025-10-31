// ============================================================================
// BAREFLOW BITCODE MODULE - Fibonacci
// ============================================================================
// Compiled to LLVM bitcode for runtime JIT
// No external dependencies (freestanding)
// ============================================================================

int module_fibonacci(void) {
    int a = 0;
    int b = 1;

    for (int i = 0; i < 20; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }

    return a;  // Returns 6765
}
