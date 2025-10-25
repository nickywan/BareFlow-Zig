// ============================================================================
// BAREFLOW - Embedded Modules
// ============================================================================
// File: kernel/embedded_modules.h
// Purpose: Module definitions embedded in kernel
// Note: __attribute__((noinline, used)) prevents aggressive optimization
// ============================================================================

#ifndef EMBEDDED_MODULES_H
#define EMBEDDED_MODULES_H

#include <stdint.h>
#include "module_loader.h"

// ============================================================================
// EMBEDDED MODULE: fibonacci
// ============================================================================

// Prevent inlining to ensure function pointer is valid
__attribute__((noinline, used))
static int module_fibonacci(void) {
    int a = 0, b = 1;
    for (int i = 0; i < 20; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return a;  // Returns 6765
}

static const module_header_t fibonacci_header = {
    .magic = MODULE_MAGIC,
    .name = "fibonacci",
    .entry_point = (void*)module_fibonacci,
    .code_size = 128,  // Approximate
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: simple_sum
// ============================================================================

__attribute__((noinline, used))
static int module_simple_sum(void) {
    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    return sum;  // Returns 5050
}

static const module_header_t simple_sum_header = {
    .magic = MODULE_MAGIC,
    .name = "sum",
    .entry_point = (void*)module_simple_sum,
    .code_size = 96,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: compute
// ============================================================================

__attribute__((noinline, used))
static int module_compute(void) {
    int result = 0;
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            result += (i * j) % 1000;
        }
    }
    return result;
}

static const module_header_t compute_header = {
    .magic = MODULE_MAGIC,
    .name = "compute",
    .entry_point = (void*)module_compute,
    .code_size = 256,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: primes (Advanced test)
// ============================================================================

__attribute__((noinline, used))
static int module_is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;

    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

__attribute__((noinline, used))
static int module_count_primes(void) {
    int count = 0;
    for (int i = 0; i < 1000; i++) {
        if (module_is_prime(i)) {
            count++;
        }
    }
    return count;  // Returns 168 (number of primes < 1000)
}

static const module_header_t primes_header = {
    .magic = MODULE_MAGIC,
    .name = "primes",
    .entry_point = (void*)module_count_primes,
    .code_size = 384,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: fft_1d (stub - will be replaced by cache)
// ============================================================================

__attribute__((noinline, used))
static int module_fft_1d(void) {
    return 0;  // Stub - real implementation in cache
}

static const module_header_t fft_1d_header = {
    .magic = MODULE_MAGIC,
    .name = "fft_1d",
    .entry_point = (void*)module_fft_1d,
    .code_size = 1668,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: sha256 (stub - will be replaced by cache)
// ============================================================================

__attribute__((noinline, used))
static int module_sha256(void) {
    return 0;  // Stub - real implementation in cache
}

static const module_header_t sha256_header = {
    .magic = MODULE_MAGIC,
    .name = "sha256",
    .entry_point = (void*)module_sha256,
    .code_size = 1848,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: matrix_mul (stub - will be replaced by cache)
// ============================================================================

__attribute__((noinline, used))
static int module_matrix_mul(void) {
    return 0;  // Stub - real implementation in cache
}

static const module_header_t matrix_mul_header = {
    .magic = MODULE_MAGIC,
    .name = "matrix_mul",
    .entry_point = (void*)module_matrix_mul,
    .code_size = 3920,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: quicksort (stub - will be replaced by cache)
// ============================================================================

__attribute__((noinline, used))
static int module_quicksort(void) {
    return 0;  // Stub - real implementation in cache
}

static const module_header_t quicksort_header = {
    .magic = MODULE_MAGIC,
    .name = "quicksort",
    .entry_point = (void*)module_quicksort,
    .code_size = 1500,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: strops (stub - will be replaced by cache)
// ============================================================================

__attribute__((noinline, used))
static int module_strops(void) {
    return 0;  // Stub - real implementation in cache
}

static const module_header_t strops_header = {
    .magic = MODULE_MAGIC,
    .name = "strops",
    .entry_point = (void*)module_strops,
    .code_size = 504,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: regex_dfa (stub)
// ============================================================================

__attribute__((noinline, used))
static int module_regex_dfa(void) {
    return 0;  // Stub - real implementation in cache or disk
}

static const module_header_t regex_dfa_header = {
    .magic = MODULE_MAGIC,
    .name = "regex_dfa",
    .entry_point = (void*)module_regex_dfa,
    .code_size = 27,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: gemm_tile (stub)
// ============================================================================

__attribute__((noinline, used))
static int module_gemm_tile(void) {
    return 0;  // Stub - real implementation in cache or disk
}

static const module_header_t gemm_tile_header = {
    .magic = MODULE_MAGIC,
    .name = "gemm_tile",
    .entry_point = (void*)module_gemm_tile,
    .code_size = 24852,
    .version = 1
};

// ============================================================================
// EMBEDDED MODULE: physics_step (stub)
// ============================================================================

__attribute__((noinline, used))
static int module_physics_step(void) {
    return 0;  // Stub - real implementation in cache or disk
}

static const module_header_t physics_step_header = {
    .magic = MODULE_MAGIC,
    .name = "physics_step",
    .entry_point = (void*)module_physics_step,
    .code_size = 824,
    .version = 1
};

// ============================================================================
// MODULE REGISTRY
// ============================================================================

static const module_header_t* embedded_modules[] = {
    &fibonacci_header,
    &simple_sum_header,
    &compute_header,
    &primes_header,
    &fft_1d_header,
    &sha256_header,
    &matrix_mul_header,
    &quicksort_header,
    &strops_header,
    &regex_dfa_header,
    &gemm_tile_header,
    &physics_step_header,
    NULL
};

static inline int load_embedded_modules(module_manager_t* mgr) {
    int count = 0;
    for (int i = 0; embedded_modules[i] != NULL; i++) {
        if (module_load(mgr, embedded_modules[i], sizeof(module_header_t)) == 0) {
            count++;
        }
    }
    return count;
}

#endif // EMBEDDED_MODULES_H
