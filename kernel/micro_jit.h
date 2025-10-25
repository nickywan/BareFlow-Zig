// ============================================================================
// BAREFLOW - Micro JIT (Lightweight Alternative to LLVM)
// ============================================================================
// File: kernel/micro_jit.h
// Purpose: ~10KB x86 code generator for hot loops (vs 500KB LLVM)
// Strategy: Direct x86 code emission for critical patterns
// ============================================================================

#ifndef MICRO_JIT_H
#define MICRO_JIT_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// MICRO JIT CONTEXT
// ============================================================================

#define MAX_JIT_CODE_SIZE (8 * 1024)  // 8KB per function (sufficient for small functions)

typedef struct {
    uint8_t* code_buffer;    // Executable code buffer
    size_t code_size;        // Current size
    size_t code_capacity;    // Max capacity
    void* allocator;         // JIT allocator reference
} micro_jit_ctx_t;

// ============================================================================
// JIT OPERATIONS
// ============================================================================

typedef enum {
    JIT_OP_NOP,
    JIT_OP_MOV_REG_IMM,   // mov reg, imm32
    JIT_OP_ADD_REG_REG,   // add reg, reg
    JIT_OP_SUB_REG_REG,   // sub reg, reg
    JIT_OP_MUL_REG_REG,   // imul reg, reg
    JIT_OP_CMP_REG_IMM,   // cmp reg, imm32
    JIT_OP_JMP,           // jmp offset
    JIT_OP_JE,            // je offset (jump if equal)
    JIT_OP_JNE,           // jne offset (jump if not equal)
    JIT_OP_JL,            // jl offset (jump if less)
    JIT_OP_RET            // ret
} jit_opcode_t;

typedef enum {
    REG_EAX = 0,
    REG_ECX = 1,
    REG_EDX = 2,
    REG_EBX = 3,
    REG_ESP = 4,
    REG_EBP = 5,
    REG_ESI = 6,
    REG_EDI = 7
} x86_register_t;

// ============================================================================
// MICRO JIT API
// ============================================================================
// Note: jit_alloc_code() and jit_free_code() are provided by jit_allocator.h

/**
 * Initialize micro JIT context
 * Returns: 0 on success, -1 on error
 */
int micro_jit_init(micro_jit_ctx_t* ctx, void* jit_allocator);

/**
 * Emit x86 instruction
 */
void micro_jit_emit_mov_reg_imm(micro_jit_ctx_t* ctx, x86_register_t reg, int32_t imm);
void micro_jit_emit_add(micro_jit_ctx_t* ctx, x86_register_t dst, x86_register_t src);
void micro_jit_emit_sub(micro_jit_ctx_t* ctx, x86_register_t dst, x86_register_t src);
void micro_jit_emit_cmp_reg_imm(micro_jit_ctx_t* ctx, x86_register_t reg, int32_t imm);
void micro_jit_emit_jmp(micro_jit_ctx_t* ctx, int32_t offset);
void micro_jit_emit_je(micro_jit_ctx_t* ctx, int32_t offset);
void micro_jit_emit_ret(micro_jit_ctx_t* ctx);

/**
 * Finalize code (make executable)
 * Returns: function pointer on success, NULL on error
 */
void* micro_jit_finalize(micro_jit_ctx_t* ctx);

/**
 * Cleanup micro JIT context
 */
void micro_jit_destroy(micro_jit_ctx_t* ctx);

// ============================================================================
// HIGH-LEVEL JIT PATTERNS
// ============================================================================

/**
 * JIT compile simple loop (for i = 0; i < n; i++) body
 */
void* micro_jit_compile_loop(micro_jit_ctx_t* ctx, int n,
                              void (*body_emitter)(micro_jit_ctx_t*, int));

/**
 * JIT compile fibonacci (special case)
 * Returns function pointer: int fibonacci(void)
 */
void* micro_jit_compile_fibonacci(micro_jit_ctx_t* ctx, int iterations);

/**
 * JIT compile sum 1..n
 * Returns function pointer: int sum(void)
 */
void* micro_jit_compile_sum(micro_jit_ctx_t* ctx, int n);

#endif // MICRO_JIT_H
