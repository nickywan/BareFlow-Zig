/**
 * JIT Runtime - Public API
 *
 * JIT profiling and optimization interface for self-optimizing applications.
 */

#ifndef JIT_RUNTIME_H
#define JIT_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// JIT PROFILING
// ============================================================================

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
typedef struct jit_profile_t {
    jit_profile_entry_t functions[JIT_MAX_FUNCTIONS];
    int num_functions;
} jit_profile_t;

/**
 * Initialize profiling system
 */
void jit_profile_init(jit_profile_t* prof);

/**
 * Begin profiling a function
 * Call at the start of the function you want to profile
 */
void jit_profile_begin(jit_profile_t* prof, const char* func_name);

/**
 * End profiling a function
 * Call at the end of the function
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

// ============================================================================
// JIT OPTIMIZATION (Phase 3 - Meta-Circular)
// ============================================================================

/**
 * Optimize hot functions
 * Placeholder for Phase 3 - Meta-Circular JIT compiler
 */
void jit_optimize_hot_functions(void);

/**
 * Set optimization thresholds
 * warm: calls before O1
 * hot: calls before O2
 * very_hot: calls before O3
 */
void jit_set_threshold(int warm, int hot, int very_hot);

/**
 * Get current optimization level for a function
 * Returns: 0 (none), 1 (O1), 2 (O2), 3 (O3)
 */
int jit_get_optimization_level(const char* func_name);

#ifdef __cplusplus
}
#endif

#endif // JIT_RUNTIME_H
