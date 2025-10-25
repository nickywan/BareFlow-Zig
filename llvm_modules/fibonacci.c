// ============================================================================
// LLVM Test Module: Fibonacci
// ============================================================================
// This module will be compiled to LLVM bitcode, then to ELF at different
// optimization levels (O0, O1, O2, O3) to demonstrate adaptive JIT.

int fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Iterative version for comparison
int fibonacci_iter(int n) {
    if (n <= 1) return n;

    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// Entry point for testing
int compute(void) {
    return fibonacci(10);  // Returns 55
}
