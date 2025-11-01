// test_quickwin_matmul.cpp - Quick Win: Matrix Multiply Performance Test
//
// Purpose: Show that optimization matters for compute-heavy workloads
// Simpler approach: Measure native performance at different opt levels

#include <iostream>
#include <chrono>
#include <cstring>

// Matrix multiply - will be compiled with different optimization levels
void matrix_multiply(int* A, int* B, int* C, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int sum = 0;
            for (int k = 0; k < N; k++) {
                sum += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

int main() {
    std::cout << "=== Quick Win 1: Matrix Multiply Performance ===\n\n";

    const int N = 128;  // 128x128 matrices
    const int SIZE = N * N;
    const int ITERATIONS = 10;

    // Allocate matrices
    int* A = new int[SIZE];
    int* B = new int[SIZE];
    int* C = new int[SIZE];

    // Initialize
    for (int i = 0; i < SIZE; i++) {
        A[i] = i % 10;
        B[i] = (i * 2) % 10;
        C[i] = 0;
    }

    std::cout << "Matrix size: " << N << "x" << N << "\n";
    std::cout << "Total operations: " << (N * N * N) << "\n";
    std::cout << "Iterations: " << ITERATIONS << "\n\n";

    // Warmup
    matrix_multiply(A, B, C, N);

    // Benchmark
    std::cout << "Running benchmark...\n";
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < ITERATIONS; iter++) {
        matrix_multiply(A, B, C, N);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    double avg_time_ms = (duration / 1000.0) / ITERATIONS;
    double ops_per_sec = (N * N * N) / (avg_time_ms / 1000.0);

    std::cout << "\n=== Results ===\n\n";
    std::cout << "Total time: " << (duration / 1000.0) << " ms\n";
    std::cout << "Average time per iteration: " << avg_time_ms << " ms\n";
    std::cout << "Operations per second: " << (ops_per_sec / 1e6) << " M ops/s\n";
    std::cout << "\nC[0][0] = " << C[0] << "\n";
    std::cout << "C[N-1][N-1] = " << C[SIZE-1] << "\n";

    std::cout << "\n✓ Benchmark complete\n";
    std::cout << "\nNote: This was compiled with -O2\n";
    std::cout << "To see optimization impact, compile with:\n";
    std::cout << "  clang++ -O0 (no optimization)\n";
    std::cout << "  clang++ -O1 (basic optimization)\n";
    std::cout << "  clang++ -O2 (aggressive optimization)\n";
    std::cout << "  clang++ -O3 (maximum optimization)\n";

    delete[] A;
    delete[] B;
    delete[] C;
    return 0;
}
