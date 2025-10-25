// ============================================================================
// Matrix Multiplication - Compute-Intensive Test Module
// ============================================================================
// Matrix multiplication is perfect for PGO because:
// - Triple nested loops (hot paths)
// - Predictable access patterns
// - Benefits from loop unrolling and vectorization

#define MATRIX_SIZE 8

// Matrix multiplication: C = A * B
static void matrix_multiply(int A[MATRIX_SIZE][MATRIX_SIZE],
                           int B[MATRIX_SIZE][MATRIX_SIZE],
                           int C[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            int sum = 0;
            for (int k = 0; k < MATRIX_SIZE; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

// Initialize matrix with test data
static void init_matrix(int M[MATRIX_SIZE][MATRIX_SIZE], int seed) {
    int val = seed;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            M[i][j] = (val * 13 + 7) % 100;
            val++;
        }
    }
}

// Compute checksum to verify result
static int checksum(int M[MATRIX_SIZE][MATRIX_SIZE]) {
    int sum = 0;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            sum += M[i][j];
        }
    }
    return sum;
}

// Entry point - will be called 1500+ times for HOT classification
int compute(void) {
    static int A[MATRIX_SIZE][MATRIX_SIZE];
    static int B[MATRIX_SIZE][MATRIX_SIZE];
    static int C[MATRIX_SIZE][MATRIX_SIZE];

    init_matrix(A, 42);
    init_matrix(B, 17);
    matrix_multiply(A, B, C);

    return checksum(C);
}
