#include "cache_loader.h"

extern const unsigned char cache_module_compute[];
extern const unsigned int cache_module_compute_size;
extern const unsigned char cache_module_fft_1d[];
extern const unsigned int cache_module_fft_1d_size;
extern const unsigned char cache_module_fibonacci[];
extern const unsigned int cache_module_fibonacci_size;
extern const unsigned char cache_module_matrix_mul[];
extern const unsigned int cache_module_matrix_mul_size;
extern const unsigned char cache_module_primes[];
extern const unsigned int cache_module_primes_size;
extern const unsigned char cache_module_sum[];
extern const unsigned int cache_module_sum_size;

void cache_registry_foreach(cache_registry_iter_fn fn, void* ctx) {
    if (!fn) {
        return;
    }
    fn("compute", cache_module_compute, cache_module_compute_size, ctx);
    fn("fft_1d", cache_module_fft_1d, cache_module_fft_1d_size, ctx);
    fn("fibonacci", cache_module_fibonacci, cache_module_fibonacci_size, ctx);
    fn("matrix_mul", cache_module_matrix_mul, cache_module_matrix_mul_size, ctx);
    fn("primes", cache_module_primes, cache_module_primes_size, ctx);
    fn("sum", cache_module_sum, cache_module_sum_size, ctx);
}
