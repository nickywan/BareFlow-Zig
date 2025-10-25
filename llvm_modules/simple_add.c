// ============================================================================
// Simple LLVM Bitcode Module: Addition
// ============================================================================

int add_numbers(int a, int b) {
    return a + b;
}

int multiply_by_10(int x) {
    return x * 10;
}

// Entry point - computes 5 + 3 = 8
int compute(void) {
    int result = add_numbers(5, 3);
    return multiply_by_10(result);  // Returns 80
}
