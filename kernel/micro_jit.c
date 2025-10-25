// ============================================================================
// BAREFLOW - Micro JIT Implementation
// ============================================================================

#include "micro_jit.h"
#include "jit_allocator.h"
#include "stdlib.h"

// Emit single byte
static void emit_byte(micro_jit_ctx_t* ctx, uint8_t byte) {
    if (ctx->code_size < ctx->code_capacity) {
        ctx->code_buffer[ctx->code_size++] = byte;
    }
}

// Emit 32-bit value (little-endian)
static void emit_dword(micro_jit_ctx_t* ctx, int32_t value) {
    emit_byte(ctx, value & 0xFF);
    emit_byte(ctx, (value >> 8) & 0xFF);
    emit_byte(ctx, (value >> 16) & 0xFF);
    emit_byte(ctx, (value >> 24) & 0xFF);
}

// Initialize micro JIT context
int micro_jit_init(micro_jit_ctx_t* ctx, void* jit_allocator) {
    if (!ctx) return -1;

    // Allocate code buffer from JIT allocator
    ctx->code_buffer = (uint8_t*)jit_alloc_code(MAX_JIT_CODE_SIZE);
    if (!ctx->code_buffer) {
        return -1;
    }

    ctx->code_size = 0;
    ctx->code_capacity = MAX_JIT_CODE_SIZE;
    ctx->allocator = jit_allocator;

    return 0;
}

// MOV reg, imm32
void micro_jit_emit_mov_reg_imm(micro_jit_ctx_t* ctx, x86_register_t reg, int32_t imm) {
    // Opcode: B8+r for mov eax/ecx/edx/etc, imm32
    emit_byte(ctx, 0xB8 + reg);
    emit_dword(ctx, imm);
}

// ADD dst, src
void micro_jit_emit_add(micro_jit_ctx_t* ctx, x86_register_t dst, x86_register_t src) {
    // Opcode: 01 /r (add r/m32, r32)
    emit_byte(ctx, 0x01);
    // ModR/M: 11 (reg-reg), src, dst
    emit_byte(ctx, 0xC0 | (src << 3) | dst);
}

// SUB dst, src
void micro_jit_emit_sub(micro_jit_ctx_t* ctx, x86_register_t dst, x86_register_t src) {
    // Opcode: 29 /r (sub r/m32, r32)
    emit_byte(ctx, 0x29);
    emit_byte(ctx, 0xC0 | (src << 3) | dst);
}

// CMP reg, imm32
void micro_jit_emit_cmp_reg_imm(micro_jit_ctx_t* ctx, x86_register_t reg, int32_t imm) {
    // Opcode: 81 /7 (cmp r/m32, imm32)
    emit_byte(ctx, 0x81);
    emit_byte(ctx, 0xF8 + reg);  // ModR/M: 11 111 reg
    emit_dword(ctx, imm);
}

// JMP offset
void micro_jit_emit_jmp(micro_jit_ctx_t* ctx, int32_t offset) {
    // Opcode: E9 (jmp rel32)
    emit_byte(ctx, 0xE9);
    emit_dword(ctx, offset);
}

// JE offset (jump if equal/zero)
void micro_jit_emit_je(micro_jit_ctx_t* ctx, int32_t offset) {
    // Opcode: 0F 84 (je rel32)
    emit_byte(ctx, 0x0F);
    emit_byte(ctx, 0x84);
    emit_dword(ctx, offset);
}

// RET
void micro_jit_emit_ret(micro_jit_ctx_t* ctx) {
    // Opcode: C3
    emit_byte(ctx, 0xC3);
}

// Finalize code (return function pointer)
void* micro_jit_finalize(micro_jit_ctx_t* ctx) {
    if (!ctx || !ctx->code_buffer || ctx->code_size == 0) {
        return NULL;
    }

    // Code is already in executable memory (JIT allocator)
    // Just return pointer
    return (void*)ctx->code_buffer;
}

// Cleanup
void micro_jit_destroy(micro_jit_ctx_t* ctx) {
    if (!ctx) return;

    if (ctx->code_buffer) {
        jit_free_code(ctx->code_buffer);
        ctx->code_buffer = NULL;
    }

    ctx->code_size = 0;
    ctx->code_capacity = 0;
}

// ============================================================================
// HIGH-LEVEL PATTERNS
// ============================================================================

// Compile fibonacci(void) -> int
void* micro_jit_compile_fibonacci(micro_jit_ctx_t* ctx, int iterations) {
    // Reset context
    ctx->code_size = 0;

    // int a = 0, b = 1;
    micro_jit_emit_mov_reg_imm(ctx, REG_EAX, 0);  // a = 0
    micro_jit_emit_mov_reg_imm(ctx, REG_ECX, 1);  // b = 1
    micro_jit_emit_mov_reg_imm(ctx, REG_EDX, 0);  // i = 0

    // loop:
    size_t loop_start = ctx->code_size;

    // if (i >= iterations) goto end
    micro_jit_emit_cmp_reg_imm(ctx, REG_EDX, iterations);
    // JGE = jump if greater or equal (we'll patch offset later)
    size_t jge_offset_pos = ctx->code_size;
    emit_byte(ctx, 0x0F);  // JGE opcode part 1
    emit_byte(ctx, 0x8D);  // JGE opcode part 2
    emit_dword(ctx, 0);    // Placeholder for offset

    // temp = a + b;
    // mov ebx, eax (temp = a)
    emit_byte(ctx, 0x89);
    emit_byte(ctx, 0xC3);
    micro_jit_emit_add(ctx, REG_EBX, REG_ECX);  // temp += b

    // a = b; b = temp;
    emit_byte(ctx, 0x89);  // mov eax, ecx (a = b)
    emit_byte(ctx, 0xC8);
    emit_byte(ctx, 0x89);  // mov ecx, ebx (b = temp)
    emit_byte(ctx, 0xD9);

    // i++
    emit_byte(ctx, 0xFF);  // inc edx
    emit_byte(ctx, 0xC2);

    // jmp loop
    int32_t jmp_offset = loop_start - (ctx->code_size + 5);
    micro_jit_emit_jmp(ctx, jmp_offset);

    // end: (patch JGE offset)
    size_t end_pos = ctx->code_size;
    int32_t jge_offset = end_pos - (jge_offset_pos + 6);
    // Patch offset
    ctx->code_buffer[jge_offset_pos + 2] = jge_offset & 0xFF;
    ctx->code_buffer[jge_offset_pos + 3] = (jge_offset >> 8) & 0xFF;
    ctx->code_buffer[jge_offset_pos + 4] = (jge_offset >> 16) & 0xFF;
    ctx->code_buffer[jge_offset_pos + 5] = (jge_offset >> 24) & 0xFF;

    // Return a
    // (eax already contains result)
    micro_jit_emit_ret(ctx);

    return micro_jit_finalize(ctx);
}

// Compile sum(1..n) -> int
void* micro_jit_compile_sum(micro_jit_ctx_t* ctx, int n) {
    ctx->code_size = 0;

    // sum = 0, i = 1
    micro_jit_emit_mov_reg_imm(ctx, REG_EAX, 0);  // sum
    micro_jit_emit_mov_reg_imm(ctx, REG_ECX, 1);  // i

    // loop:
    size_t loop_start = ctx->code_size;

    // if (i > n) goto end
    micro_jit_emit_cmp_reg_imm(ctx, REG_ECX, n);
    size_t jg_offset_pos = ctx->code_size;
    emit_byte(ctx, 0x0F);  // JG
    emit_byte(ctx, 0x8F);
    emit_dword(ctx, 0);

    // sum += i
    micro_jit_emit_add(ctx, REG_EAX, REG_ECX);

    // i++
    emit_byte(ctx, 0xFF);
    emit_byte(ctx, 0xC1);

    // jmp loop
    int32_t jmp_offset = loop_start - (ctx->code_size + 5);
    micro_jit_emit_jmp(ctx, jmp_offset);

    // end: patch JG
    size_t end_pos = ctx->code_size;
    int32_t jg_offset = end_pos - (jg_offset_pos + 6);
    ctx->code_buffer[jg_offset_pos + 2] = jg_offset & 0xFF;
    ctx->code_buffer[jg_offset_pos + 3] = (jg_offset >> 8) & 0xFF;
    ctx->code_buffer[jg_offset_pos + 4] = (jg_offset >> 16) & 0xFF;
    ctx->code_buffer[jg_offset_pos + 5] = (jg_offset >> 24) & 0xFF;

    micro_jit_emit_ret(ctx);

    return micro_jit_finalize(ctx);
}
