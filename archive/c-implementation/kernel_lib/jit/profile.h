/**
 * JIT Profiling System - Implementation Header
 *
 * Simple function profiling with cycle counting (rdtsc).
 * Tracks call count, total cycles, min/max/avg for each function.
 *
 * NOTE: This header includes ../jit_runtime.h for type definitions.
 *       Users should include jit_runtime.h, not this header directly.
 */

#ifndef JIT_PROFILE_H
#define JIT_PROFILE_H

#include "../jit_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

// Types defined in jit_runtime.h:
// - jit_profile_t
// - jit_profile_entry_t
// - JIT_MAX_FUNCTIONS
// - JIT_MAX_FUNC_NAME

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
