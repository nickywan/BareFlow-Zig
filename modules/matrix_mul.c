// Module: Matrix multiplication benchmark (64x64)

#include <stdint.h>

#define MODULE_MAGIC 0x4D4F4442

typedef struct {
    uint32_t magic;
    char name[32];
    void* entry_point;
    uint32_t code_size;
    uint32_t version;
} __attribute__((packed)) module_header_t;

#define MATRIX_N 64

static int mat_a[MATRIX_N][MATRIX_N];
static int mat_b[MATRIX_N][MATRIX_N];
static int mat_c[MATRIX_N][MATRIX_N];

static void init_matrices(void) {
    for (int i = 0; i < MATRIX_N; ++i) {
        for (int j = 0; j < MATRIX_N; ++j) {
            mat_a[i][j] = (i + j) % 17;
            mat_b[i][j] = (i * 3 + j * 5) % 19;
            mat_c[i][j] = 0;
        }
    }
}

static int multiply(void) {
    init_matrices();
    for (int i = 0; i < MATRIX_N; ++i) {
        for (int k = 0; k < MATRIX_N; ++k) {
            int a = mat_a[i][k];
            for (int j = 0; j < MATRIX_N; ++j) {
                mat_c[i][j] += a * mat_b[k][j];
            }
        }
    }

    int checksum = 0;
    for (int i = 0; i < MATRIX_N; ++i) {
        for (int j = 0; j < MATRIX_N; ++j) {
            checksum ^= mat_c[i][j];
        }
    }
    return checksum;
}

__attribute__((section(".module_header")))
const module_header_t module_info = {
    .magic = MODULE_MAGIC,
    .name = "matrix_mul",
    .entry_point = (void*)multiply,
    .code_size = 0,
    .version = 1
};
