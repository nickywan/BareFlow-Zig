// ============================================================================
// BAREFLOW MODULE - Tiled GEMM (General Matrix Multiply)
// ============================================================================
// Purpose: Cache-aware matrix multiplication with tiling
// Tests: Cache behavior, memory access patterns, allocator pressure
// ============================================================================

#define N 32       // Matrix size (32x32)
#define TILE 8     // Tile size (8x8 blocks)

// Matrices in .data section (initialized)
static int A[N][N] = {{1}};  // Initialize first element to force .data
static int B[N][N] = {{1}};
static int C[N][N] = {{0}};

// Initialize matrices
static void init_matrices(void) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (i + j) % 10;
            B[i][j] = (i - j) % 10;
            C[i][j] = 0;
        }
    }
}

// Tiled matrix multiplication (cache-friendly)
static void gemm_tiled(void) {
    // Zero output
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 0;
        }
    }

    // Tiled multiply
    for (int ii = 0; ii < N; ii += TILE) {
        for (int jj = 0; jj < N; jj += TILE) {
            for (int kk = 0; kk < N; kk += TILE) {
                // Multiply tile
                for (int i = ii; i < ii + TILE && i < N; i++) {
                    for (int j = jj; j < jj + TILE && j < N; j++) {
                        int sum = C[i][j];
                        for (int k = kk; k < kk + TILE && k < N; k++) {
                            sum += A[i][k] * B[k][j];
                        }
                        C[i][j] = sum;
                    }
                }
            }
        }
    }
}

// Module entry point
int module_gemm_tile(void) {
    init_matrices();

    // Run tiled GEMM multiple times
    for (int iter = 0; iter < 5; iter++) {
        gemm_tiled();
    }

    // Return checksum
    int checksum = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            checksum += C[i][j];
        }
    }

    return checksum;
}
