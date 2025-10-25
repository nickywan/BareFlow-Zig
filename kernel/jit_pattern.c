// ============================================================================
// BAREFLOW - JIT Pattern Compiler
// ============================================================================

#include "jit_pattern.h"
#include "micro_jit.h"
#include "jit_allocator.h"
#include "profiling_export.h"

// Compile a pattern to x86 code
void* jit_compile_pattern(const jit_pattern_desc_t* pattern) {
    if (!pattern) return NULL;

    micro_jit_ctx_t ctx;
    micro_jit_init(&ctx, NULL);

    void* code = NULL;

    switch (pattern->type) {
        case PATTERN_FIBONACCI:
            // Use existing micro_jit_compile_fibonacci
            code = micro_jit_compile_fibonacci(&ctx, pattern->param1);
            break;

        case PATTERN_SUM:
            // Use existing micro_jit_compile_sum
            code = micro_jit_compile_sum(&ctx, pattern->param1);
            break;

        case PATTERN_FACTORIAL:
        case PATTERN_POWER:
        case PATTERN_LOOP_ADD:
        case PATTERN_CUSTOM:
            // Not implemented yet
            serial_puts("[JIT-PATTERN] Pattern type not implemented yet\n");
            return NULL;

        default:
            serial_puts("[JIT-PATTERN] Unknown pattern type\n");
            return NULL;
    }

    if (!code) {
        serial_puts("[JIT-PATTERN] Compilation failed\n");
        return NULL;
    }

    serial_puts("[JIT-PATTERN] Compiled pattern type ");
    serial_putchar('0' + pattern->type);
    serial_puts(" at O");
    serial_putchar('0' + pattern->opt_level);
    serial_puts("\n");

    return code;
}

// Execute a compiled pattern
int jit_execute_pattern(void* compiled_func) {
    if (!compiled_func) return -1;

    typedef int (*func_ptr_t)(void);
    func_ptr_t func = (func_ptr_t)compiled_func;

    return func();
}

// Free compiled pattern code
void jit_free_pattern(void* compiled_func) {
    if (!compiled_func) return;

    // Free using JIT allocator
    jit_free_code(compiled_func);
}
