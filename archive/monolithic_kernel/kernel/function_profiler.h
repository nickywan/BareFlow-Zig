// ============================================================================
// BAREFLOW - Function-Level Profiling for JIT Optimization
// ============================================================================
// File: kernel/function_profiler.h
// Purpose: Per-function profiling with hot-path detection for JIT triggers
// ============================================================================

#ifndef FUNCTION_PROFILER_H
#define FUNCTION_PROFILER_H

#include <stdint.h>
#include <stdbool.h>

// Maximum number of functions we can track
#define MAX_FUNCTIONS 128

// JIT recompilation thresholds
#define JIT_THRESHOLD_O1  100   // After 100 calls, recompile with -O1
#define JIT_THRESHOLD_O2  1000  // After 1000 calls, recompile with -O2
#define JIT_THRESHOLD_O3  10000 // After 10000 calls, recompile with -O3

// Optimization levels
typedef enum {
    OPT_LEVEL_O0 = 0,  // No optimization
    OPT_LEVEL_O1 = 1,  // Basic optimization
    OPT_LEVEL_O2 = 2,  // Moderate optimization
    OPT_LEVEL_O3 = 3   // Aggressive optimization
} opt_level_t;

// Function profile structure
typedef struct {
    const char* name;           // Function name (e.g., "module_fibonacci::fib")
    const char* module_name;    // Parent module name (e.g., "fibonacci")
    void* address;              // Function address (for replacement)
    uint64_t call_count;        // Number of times called
    uint64_t total_cycles;      // Total cycles spent in function
    uint64_t min_cycles;        // Minimum cycles per call
    uint64_t max_cycles;        // Maximum cycles per call
    opt_level_t opt_level;      // Current optimization level
    bool needs_recompile;       // JIT recompilation flag
    bool is_hot;                // Hot path indicator
} function_profile_t;

// Function profiler manager
typedef struct {
    function_profile_t functions[MAX_FUNCTIONS];
    int function_count;
    uint64_t total_calls;       // Total function calls across all functions
    bool jit_enabled;           // Enable JIT recompilation triggers
} function_profiler_t;

// ============================================================================
// API Functions
// ============================================================================

/**
 * Initialize the function profiler
 */
void function_profiler_init(function_profiler_t* profiler, bool enable_jit);

/**
 * Register a function for profiling
 * Returns: function ID (index) or -1 on error
 */
int function_profiler_register(
    function_profiler_t* profiler,
    const char* func_name,
    const char* module_name,
    void* address
);

/**
 * Record a function call with cycle count
 * This should be called after each function execution
 */
void function_profiler_record(
    function_profiler_t* profiler,
    int func_id,
    uint64_t cycles
);

/**
 * Check if a function needs JIT recompilation
 * Returns true if call count crossed a threshold
 */
bool function_profiler_needs_recompile(
    function_profiler_t* profiler,
    int func_id
);

/**
 * Mark a function as recompiled with new optimization level
 */
void function_profiler_mark_recompiled(
    function_profiler_t* profiler,
    int func_id,
    opt_level_t new_level
);

/**
 * Get hot functions (top N by total cycles)
 * Returns: number of hot functions found
 */
int function_profiler_get_hot_functions(
    function_profiler_t* profiler,
    int* hot_func_ids,  // Output array (must be MAX_FUNCTIONS size)
    int max_count
);

/**
 * Print profiling statistics (for debugging)
 */
void function_profiler_print_stats(function_profiler_t* profiler);

/**
 * Export profiling data to JSON format
 * Returns: JSON string (caller must free)
 */
char* function_profiler_export_json(function_profiler_t* profiler);

// ============================================================================
// Helper Macros for Function Wrapping
// ============================================================================

// Macro to wrap a function call with profiling
#define PROFILE_FUNCTION_CALL(profiler, func_id, func_call) \
    do { \
        uint64_t start_cycles, end_cycles; \
        __asm__ __volatile__("rdtsc" : "=A"(start_cycles)); \
        func_call; \
        __asm__ __volatile__("rdtsc" : "=A"(end_cycles)); \
        function_profiler_record(profiler, func_id, end_cycles - start_cycles); \
    } while(0)

// Example usage:
// int result;
// PROFILE_FUNCTION_CALL(&profiler, fib_id, result = fibonacci(20));

#endif // FUNCTION_PROFILER_H
