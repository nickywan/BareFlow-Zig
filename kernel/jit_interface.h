// jit_interface.h - JIT abstraction layer
// Isolates LLVM implementation details from kernel code

#ifndef JIT_INTERFACE_H
#define JIT_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types
typedef struct JITContext JITContext;
typedef struct JITModule JITModule;

// JIT initialization
JITContext* jit_create(void);
void jit_destroy(JITContext* ctx);

// Module loading
JITModule* jit_load_bitcode(JITContext* ctx, const char* path);
JITModule* jit_load_bitcode_memory(JITContext* ctx, const uint8_t* data, size_t size);
void jit_unload_module(JITModule* mod);

// Function lookup
void* jit_find_function(JITContext* ctx, const char* name);

// Compilation
typedef enum {
    JIT_OPT_NONE = 0,      // -O0
    JIT_OPT_BASIC = 1,     // -O1
    JIT_OPT_AGGRESSIVE = 2 // -O2/-O3
} JITOptLevel;

int jit_recompile_function(JITContext* ctx, const char* name, JITOptLevel opt);

// Function profiling info
typedef struct {
    char name[64];
    void* code_ptr;
    uint64_t call_count;
    uint64_t total_cycles;
    uint32_t code_size;
    JITOptLevel current_opt_level;
} JITFunctionInfo;

// Global stats
typedef struct {
    uint64_t functions_compiled;
    uint64_t total_compile_time_us;
    uint64_t memory_used_bytes;
    uint64_t total_function_calls;
    uint64_t reoptimizations;
} JITStats;

void jit_get_stats(JITContext* ctx, JITStats* stats);

// Function profiling
#define JIT_PROFILE_THRESHOLD 100  // Calls before auto-reoptimization

// Get info for a specific function
int jit_get_function_info(JITContext* ctx, const char* name, JITFunctionInfo* info);

// List all loaded functions
int jit_list_functions(JITContext* ctx, JITFunctionInfo* infos, int max_count);

// Profile-guided reoptimization
// Returns 1 if reoptimized, 0 if not needed, -1 on error
int jit_auto_optimize(JITContext* ctx, const char* name);

// Record function call (for profiling)
void jit_record_call(JITContext* ctx, const char* name, uint64_t cycles);

// Error handling
const char* jit_get_last_error(JITContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // JIT_INTERFACE_H