/**
 * JIT Profiling System
 *
 * Simple function profiling with cycle counting (rdtsc).
 * Tracks call count, total cycles, min/max/avg for each function.
 */

#ifndef JIT_PROFILE_H
#define JIT_PROFILE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JIT_MAX_FUNCTIONS 32
#define JIT_MAX_FUNC_NAME 32

/**
 * Function profiling data
 */
typedef struct {
    char name[JIT_MAX_FUNC_NAME];
    uint64_t call_count;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t last_start;  // Start timestamp for current call
    int active;           // 1 if currently being profiled
} jit_profile_entry_t;

/**
 * Global profiling state
 */
typedef struct {
    jit_profile_entry_t functions[JIT_MAX_FUNCTIONS];
    int num_functions;
} jit_profile_t;

/**
 * Initialize profiling system
 */
void jit_profile_init(jit_profile_t* prof);

/**
 * Begin profiling a function
 * Starts cycle counter for the named function
 */
void jit_profile_begin(jit_profile_t* prof, const char* func_name);

/**
 * End profiling a function
 * Records cycle count and updates statistics
 */
void jit_profile_end(jit_profile_t* prof, const char* func_name);

/**
 * Get call count for a function
 */
uint64_t jit_get_call_count(jit_profile_t* prof, const char* func_name);

/**
 * Get average cycles for a function
 */
uint64_t jit_get_avg_cycles(jit_profile_t* prof, const char* func_name);

/**
 * Print profiling statistics for a function
 */
void jit_print_stats(jit_profile_t* prof, const char* func_name);

/**
 * Print profiling statistics for all functions
 */
void jit_print_all_stats(jit_profile_t* prof);

#ifdef __cplusplus
}
#endif

#endif // JIT_PROFILE_H
