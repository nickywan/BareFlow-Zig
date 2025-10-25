/**
 * CPU Features and Utilities
 *
 * Wrappers for CPU-specific instructions (rdtsc, cpuid, etc.)
 */

#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read Time-Stamp Counter (RDTSC)
 * Returns 64-bit cycle count since boot
 */
static inline uint64_t cpu_rdtsc(void) {
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

/**
 * CPUID instruction wrapper
 * leaf: CPUID function number
 * eax, ebx, ecx, edx: output registers (can be NULL if not needed)
 */
static inline void cpu_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx,
                              uint32_t* ecx, uint32_t* edx) {
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid"
                     : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                     : "a"(leaf));
    if (eax) *eax = a;
    if (ebx) *ebx = b;
    if (ecx) *ecx = c;
    if (edx) *edx = d;
}

/**
 * Check if CPU supports SSE
 */
static inline int cpu_has_sse(void) {
    uint32_t edx;
    cpu_cpuid(1, NULL, NULL, NULL, &edx);
    return (edx & (1 << 25)) != 0;
}

/**
 * Check if CPU supports SSE2
 */
static inline int cpu_has_sse2(void) {
    uint32_t edx;
    cpu_cpuid(1, NULL, NULL, NULL, &edx);
    return (edx & (1 << 26)) != 0;
}

/**
 * Check if CPU supports AVX
 */
static inline int cpu_has_avx(void) {
    uint32_t ecx;
    cpu_cpuid(1, NULL, NULL, &ecx, NULL);
    return (ecx & (1 << 28)) != 0;
}

#ifdef __cplusplus
}
#endif

#endif // CPU_FEATURES_H
