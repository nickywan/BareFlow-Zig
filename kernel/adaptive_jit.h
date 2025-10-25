/**
 * Adaptive JIT System - Hot-Path Detection and Recompilation
 *
 * Combines function_profiler with Micro-JIT to implement adaptive optimization:
 * 1. Profile function calls and cycle counts
 * 2. Detect hot paths (100/1000/10000 call thresholds)
 * 3. Trigger JIT recompilation at higher optimization levels
 * 4. Atomically swap code pointers for zero-downtime optimization
 */

#ifndef ADAPTIVE_JIT_H
#define ADAPTIVE_JIT_H

#include <stdint.h>
#include <stdbool.h>
#include "function_profiler.h"
#include "micro_jit.h"
#include "jit_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ADAPTIVE JIT CONTEXT
// ============================================================================

#define MAX_JIT_FUNCTIONS 32

/**
 * JIT-compiled function entry
 */
typedef struct {
    int profiler_id;             // Function ID in profiler
    void* code_v0;               // O0 version (initial)
    void* code_v1;               // O1 version (100+ calls)
    void* code_v2;               // O2 version (1000+ calls)
    void* code_v3;               // O3 version (10000+ calls)
    void* current_code;          // Active version (atomic pointer)
    micro_jit_ctx_t jit_ctx;     // JIT context for this function
    opt_level_t compiled_level;  // Highest compiled optimization level
    bool is_active;              // Entry in use
} jit_function_entry_t;

/**
 * Adaptive JIT manager
 */
typedef struct {
    function_profiler_t profiler;
    jit_function_entry_t functions[MAX_JIT_FUNCTIONS];
    int function_count;
    bool enabled;
} adaptive_jit_t;

// ============================================================================
// API FUNCTIONS
// ============================================================================

/**
 * Initialize adaptive JIT system
 */
int adaptive_jit_init(adaptive_jit_t* ajit);

/**
 * Shutdown adaptive JIT system
 */
void adaptive_jit_shutdown(adaptive_jit_t* ajit);

/**
 * Register a function for adaptive JIT compilation
 * Returns: function ID or -1 on error
 */
int adaptive_jit_register_function(
    adaptive_jit_t* ajit,
    const char* func_name,
    const char* module_name,
    void* initial_code
);

/**
 * Execute a function with profiling and adaptive recompilation
 * This wraps the function call with cycle counting and triggers
 * recompilation when thresholds are reached.
 *
 * Returns: function result
 */
int adaptive_jit_execute(adaptive_jit_t* ajit, int func_id);

/**
 * Check for hot functions and trigger recompilation
 * Call this periodically or after N function calls
 */
void adaptive_jit_check_and_recompile(adaptive_jit_t* ajit);

/**
 * Manually trigger recompilation of a function to next optimization level
 */
int adaptive_jit_recompile_function(adaptive_jit_t* ajit, int func_id);

/**
 * Atomically swap function code pointer (zero-downtime optimization)
 */
void adaptive_jit_swap_code(jit_function_entry_t* entry, void* new_code, opt_level_t new_level);

/**
 * Get current optimization level for a function
 */
opt_level_t adaptive_jit_get_opt_level(adaptive_jit_t* ajit, int func_id);

/**
 * Get profiling statistics for a function
 */
function_profile_t* adaptive_jit_get_profile(adaptive_jit_t* ajit, int func_id);

#ifdef __cplusplus
}
#endif

#endif // ADAPTIVE_JIT_H
