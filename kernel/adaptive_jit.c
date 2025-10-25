/**
 * Adaptive JIT Implementation - Hot-Path Detection and Atomic Code Swapping
 */

#include "adaptive_jit.h"
#include "profiling_export.h"
#include <stddef.h>

// External functions
extern void* memset(void* s, int c, size_t n);
extern uint64_t rdtsc(void);

// ============================================================================
// INITIALIZATION
// ============================================================================

int adaptive_jit_init(adaptive_jit_t* ajit) {
    if (!ajit) return -1;

    memset(ajit, 0, sizeof(adaptive_jit_t));

    // Initialize function profiler with JIT enabled
    function_profiler_init(&ajit->profiler, true);

    ajit->enabled = true;
    ajit->function_count = 0;

    serial_puts("[ADAPTIVE-JIT] Initialized\n");
    return 0;
}

void adaptive_jit_shutdown(adaptive_jit_t* ajit) {
    if (!ajit) return;

    // Clean up all JIT contexts
    for (int i = 0; i < ajit->function_count; i++) {
        if (ajit->functions[i].is_active) {
            micro_jit_destroy(&ajit->functions[i].jit_ctx);
        }
    }

    ajit->enabled = false;
}

// ============================================================================
// FUNCTION REGISTRATION
// ============================================================================

int adaptive_jit_register_function(
    adaptive_jit_t* ajit,
    const char* func_name,
    const char* module_name,
    void* initial_code)
{
    if (!ajit || !func_name || !module_name) return -1;
    if (ajit->function_count >= MAX_JIT_FUNCTIONS) return -1;

    // Register with profiler
    int profiler_id = function_profiler_register(
        &ajit->profiler,
        func_name,
        module_name,
        initial_code
    );

    if (profiler_id < 0) return -1;

    // Create JIT function entry
    int func_id = ajit->function_count;
    jit_function_entry_t* entry = &ajit->functions[func_id];

    entry->profiler_id = profiler_id;
    entry->code_v0 = initial_code;  // O0 version (could be AOT compiled)
    entry->code_v1 = NULL;
    entry->code_v2 = NULL;
    entry->code_v3 = NULL;
    entry->current_code = initial_code;  // Start with O0
    entry->compiled_level = OPT_LEVEL_O0;
    entry->is_active = true;

    // Initialize JIT context (we'll compile on demand)
    micro_jit_init(&entry->jit_ctx, NULL);

    ajit->function_count++;

    serial_puts("[ADAPTIVE-JIT] Registered: ");
    serial_puts(func_name);
    serial_puts("\n");

    return func_id;
}

// ============================================================================
// ATOMIC CODE SWAPPING
// ============================================================================

void adaptive_jit_swap_code(jit_function_entry_t* entry, void* new_code, opt_level_t new_level) {
    if (!entry || !new_code) return;

    // ATOMIC SWAP: On x86, pointer writes are atomic if aligned
    // This ensures zero-downtime optimization
    __atomic_store_n(&entry->current_code, new_code, __ATOMIC_RELEASE);

    // Update metadata after swap (non-critical)
    entry->compiled_level = new_level;

    serial_puts("[ATOMIC-SWAP] Code pointer updated to O");
    serial_putchar('0' + new_level);
    serial_puts("\n");
}

// ============================================================================
// EXECUTION WITH PROFILING
// ============================================================================

int adaptive_jit_execute(adaptive_jit_t* ajit, int func_id) {
    if (!ajit || func_id < 0 || func_id >= ajit->function_count) return -1;

    jit_function_entry_t* entry = &ajit->functions[func_id];
    if (!entry->is_active) return -1;

    // Get current code pointer (atomic load)
    typedef int (*func_ptr_t)(void);
    func_ptr_t func = (func_ptr_t)__atomic_load_n(&entry->current_code, __ATOMIC_ACQUIRE);

    // Profile execution
    uint64_t start = rdtsc();
    int result = func();
    uint64_t end = rdtsc();
    uint64_t cycles = end - start;

    // Record in profiler
    function_profiler_record(&ajit->profiler, entry->profiler_id, cycles);

    // Check if recompilation is needed
    if (function_profiler_needs_recompile(&ajit->profiler, entry->profiler_id)) {
        adaptive_jit_recompile_function(ajit, func_id);
    }

    return result;
}

// ============================================================================
// RECOMPILATION
// ============================================================================

int adaptive_jit_recompile_function(adaptive_jit_t* ajit, int func_id) {
    if (!ajit || func_id < 0 || func_id >= ajit->function_count) return -1;

    jit_function_entry_t* entry = &ajit->functions[func_id];
    function_profile_t* profile = &ajit->profiler.functions[entry->profiler_id];

    opt_level_t current = profile->opt_level;
    opt_level_t next_level = OPT_LEVEL_O0;

    // Determine next optimization level
    if (current == OPT_LEVEL_O0 && profile->call_count >= JIT_THRESHOLD_O1) {
        next_level = OPT_LEVEL_O1;
    } else if (current == OPT_LEVEL_O1 && profile->call_count >= JIT_THRESHOLD_O2) {
        next_level = OPT_LEVEL_O2;
    } else if (current == OPT_LEVEL_O2 && profile->call_count >= JIT_THRESHOLD_O3) {
        next_level = OPT_LEVEL_O3;
    } else {
        return 0;  // No recompilation needed
    }

    serial_puts("[RECOMPILE] ");
    serial_puts(profile->name);
    serial_puts(": O");
    serial_putchar('0' + current);
    serial_puts(" -> O");
    serial_putchar('0' + next_level);
    serial_puts("\n");

    // For demonstration: fibonacci hardcoded compilation
    // In a real system, this would invoke LLVM with different -O levels
    void* new_code = NULL;

    // Simplified: Recompile fibonacci with "optimizations"
    // In practice, this would be Micro-JIT with different code generation strategies
    if (next_level == OPT_LEVEL_O1) {
        // Compile O1 version (same as O0 for now in demo)
        new_code = micro_jit_compile_fibonacci(&entry->jit_ctx, 5);
        entry->code_v1 = new_code;
    } else if (next_level == OPT_LEVEL_O2) {
        // Compile O2 version
        new_code = micro_jit_compile_fibonacci(&entry->jit_ctx, 5);
        entry->code_v2 = new_code;
    } else if (next_level == OPT_LEVEL_O3) {
        // Compile O3 version
        new_code = micro_jit_compile_fibonacci(&entry->jit_ctx, 5);
        entry->code_v3 = new_code;
    }

    if (new_code) {
        // ATOMIC CODE SWAP - zero downtime!
        adaptive_jit_swap_code(entry, new_code, next_level);

        // Mark as recompiled in profiler
        function_profiler_mark_recompiled(&ajit->profiler, entry->profiler_id, next_level);

        return 1;  // Success
    }

    return -1;  // Compilation failed
}

void adaptive_jit_check_and_recompile(adaptive_jit_t* ajit) {
    if (!ajit) return;

    // Check all registered functions
    for (int i = 0; i < ajit->function_count; i++) {
        jit_function_entry_t* entry = &ajit->functions[i];
        if (entry->is_active && function_profiler_needs_recompile(&ajit->profiler, entry->profiler_id)) {
            adaptive_jit_recompile_function(ajit, i);
        }
    }
}

// ============================================================================
// QUERY FUNCTIONS
// ============================================================================

opt_level_t adaptive_jit_get_opt_level(adaptive_jit_t* ajit, int func_id) {
    if (!ajit || func_id < 0 || func_id >= ajit->function_count) {
        return OPT_LEVEL_O0;
    }
    return ajit->functions[func_id].compiled_level;
}

function_profile_t* adaptive_jit_get_profile(adaptive_jit_t* ajit, int func_id) {
    if (!ajit || func_id < 0 || func_id >= ajit->function_count) {
        return NULL;
    }
    return &ajit->profiler.functions[ajit->functions[func_id].profiler_id];
}
