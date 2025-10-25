// ============================================================================
// BAREFLOW - JIT Pattern Descriptor
// ============================================================================
// File: kernel/jit_pattern.h
// Purpose: Simple pattern-based code generation for Micro-JIT
//
// This is a lightweight alternative to full LLVM IR compilation.
// Instead of compiling LLVM bitcode, we use pattern descriptors that
// the Micro-JIT can quickly compile to x86 code.
// ============================================================================

#ifndef JIT_PATTERN_H
#define JIT_PATTERN_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// PATTERN TYPES
// ============================================================================

typedef enum {
    PATTERN_FIBONACCI,      // Fibonacci sequence
    PATTERN_SUM,            // Sum 1..N
    PATTERN_FACTORIAL,      // Factorial N!
    PATTERN_POWER,          // Power X^N
    PATTERN_LOOP_ADD,       // Loop adding constant
    PATTERN_CUSTOM          // Custom pattern (future)
} pattern_type_t;

// Pattern descriptor
typedef struct {
    pattern_type_t type;    // Pattern type
    uint32_t param1;        // First parameter (e.g., N for fibonacci(N))
    uint32_t param2;        // Second parameter (e.g., X for power(X,N))
    uint32_t opt_level;     // Optimization level (0-3)
} jit_pattern_desc_t;

// ============================================================================
// PATTERN API
// ============================================================================

/**
 * Compile a pattern to x86 code using Micro-JIT
 * Returns: function pointer or NULL on error
 */
void* jit_compile_pattern(const jit_pattern_desc_t* pattern);

/**
 * Execute a compiled pattern
 * Returns: result value
 */
int jit_execute_pattern(void* compiled_func);

/**
 * Free compiled pattern code
 */
void jit_free_pattern(void* compiled_func);

#endif // JIT_PATTERN_H
