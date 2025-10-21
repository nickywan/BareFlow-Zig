// jit_llvm18.cpp - LLVM 18 implementation of JIT interface
// Enhanced with profiling and adaptive optimization

#include "jit_interface.h"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Passes/PassBuilder.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

using namespace llvm;
using namespace llvm::orc;

// Function tracking for profiling
struct FunctionProfile {
    std::string name;
    void* code_ptr;
    uint64_t call_count;
    uint64_t total_cycles;
    uint32_t code_size;
    JITOptLevel current_opt_level;

    FunctionProfile() : code_ptr(nullptr), call_count(0), total_cycles(0),
                       code_size(0), current_opt_level(JIT_OPT_NONE) {}
};

struct JITContext {
    std::unique_ptr<LLJIT> jit;
    std::unique_ptr<LLVMContext> context;
    std::string last_error;
    JITStats stats;
    std::unordered_map<std::string, FunctionProfile> function_profiles;

    JITContext() : stats{0, 0, 0, 0, 0} {
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();
        InitializeNativeTargetAsmParser();

        context = std::make_unique<LLVMContext>();

        auto jit_expected = LLJITBuilder().create();
        if (jit_expected) {
            jit = std::move(*jit_expected);
        } else {
            last_error = toString(jit_expected.takeError());
        }
    }
};

struct JITModule {
    std::string name;
};

extern "C" {

JITContext* jit_create(void) {
    return new JITContext();
}

void jit_destroy(JITContext* ctx) {
    delete ctx;
}

JITModule* jit_load_bitcode(JITContext* ctx, const char* path) {
    if (!ctx || !ctx->jit) {
        return nullptr;
    }
    
    SMDiagnostic err;
    auto module = parseIRFile(path, err, *ctx->context);
    if (!module) {
        ctx->last_error = "Failed to parse IR file";
        return nullptr;
    }
    
    auto tsm = ThreadSafeModule(std::move(module), std::make_unique<LLVMContext>());
    auto add_err = ctx->jit->addIRModule(std::move(tsm));
    if (add_err) {
        ctx->last_error = toString(std::move(add_err));
        return nullptr;
    }
    
    ctx->stats.functions_compiled++;
    
    auto mod = new JITModule();
    mod->name = path;
    return mod;
}

JITModule* jit_load_bitcode_memory(JITContext* ctx, const uint8_t* data, size_t size) {
    if (!ctx || !ctx->jit) {
        return nullptr;
    }
    
    auto buf = MemoryBuffer::getMemBuffer(
        StringRef((const char*)data, size), 
        "", 
        false
    );
    
    SMDiagnostic err;
    auto module = parseIR(*buf, err, *ctx->context);
    if (!module) {
        ctx->last_error = "Failed to parse IR from memory";
        return nullptr;
    }
    
    auto tsm = ThreadSafeModule(std::move(module), std::make_unique<LLVMContext>());
    auto add_err = ctx->jit->addIRModule(std::move(tsm));
    if (add_err) {
        ctx->last_error = toString(std::move(add_err));
        return nullptr;
    }
    
    ctx->stats.functions_compiled++;
    
    auto mod = new JITModule();
    mod->name = "<memory>";
    return mod;
}

void jit_unload_module(JITModule* mod) {
    delete mod;
}

void* jit_find_function(JITContext* ctx, const char* name) {
    if (!ctx || !ctx->jit) {
        return nullptr;
    }

    auto sym = ctx->jit->lookup(name);
    if (!sym) {
        ctx->last_error = toString(sym.takeError());
        return nullptr;
    }

    void* ptr = sym->toPtr<void*>();

    // Track this function for profiling
    if (ctx->function_profiles.find(name) == ctx->function_profiles.end()) {
        FunctionProfile profile;
        profile.name = name;
        profile.code_ptr = ptr;
        profile.current_opt_level = JIT_OPT_NONE;
        ctx->function_profiles[name] = profile;
    }

    return ptr;
}

int jit_recompile_function(JITContext* ctx, const char* name, JITOptLevel opt) {
    if (!ctx || !ctx->jit) {
        if (ctx) ctx->last_error = "Invalid JIT context";
        return -1;
    }

    // Update optimization level in profile
    auto it = ctx->function_profiles.find(name);
    if (it != ctx->function_profiles.end()) {
        it->second.current_opt_level = opt;
        ctx->stats.reoptimizations++;
    }

    // Note: True recompilation with different optimization levels would require
    // reloading the module with LLVM optimization passes applied.
    // For now, we track the optimization intent for future implementation.
    return 0;
}

void jit_get_stats(JITContext* ctx, JITStats* stats) {
    if (ctx && stats) {
        *stats = ctx->stats;
    }
}

const char* jit_get_last_error(JITContext* ctx) {
    return ctx ? ctx->last_error.c_str() : "Invalid context";
}

int jit_get_function_info(JITContext* ctx, const char* name, JITFunctionInfo* info) {
    if (!ctx || !name || !info) {
        return -1;
    }

    auto it = ctx->function_profiles.find(name);
    if (it == ctx->function_profiles.end()) {
        ctx->last_error = "Function not found";
        return -1;
    }

    const FunctionProfile& profile = it->second;
    strncpy(info->name, profile.name.c_str(), sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->code_ptr = profile.code_ptr;
    info->call_count = profile.call_count;
    info->total_cycles = profile.total_cycles;
    info->code_size = profile.code_size;
    info->current_opt_level = profile.current_opt_level;

    return 0;
}

int jit_list_functions(JITContext* ctx, JITFunctionInfo* infos, int max_count) {
    if (!ctx || !infos || max_count <= 0) {
        return -1;
    }

    int count = 0;
    for (const auto& pair : ctx->function_profiles) {
        if (count >= max_count) break;

        const FunctionProfile& profile = pair.second;
        strncpy(infos[count].name, profile.name.c_str(), sizeof(infos[count].name) - 1);
        infos[count].name[sizeof(infos[count].name) - 1] = '\0';
        infos[count].code_ptr = profile.code_ptr;
        infos[count].call_count = profile.call_count;
        infos[count].total_cycles = profile.total_cycles;
        infos[count].code_size = profile.code_size;
        infos[count].current_opt_level = profile.current_opt_level;
        count++;
    }

    return count;
}

void jit_record_call(JITContext* ctx, const char* name, uint64_t cycles) {
    if (!ctx || !name) {
        return;
    }

    auto it = ctx->function_profiles.find(name);
    if (it != ctx->function_profiles.end()) {
        it->second.call_count++;
        it->second.total_cycles += cycles;
        ctx->stats.total_function_calls++;
    }
}

int jit_auto_optimize(JITContext* ctx, const char* name) {
    if (!ctx || !name) {
        return -1;
    }

    auto it = ctx->function_profiles.find(name);
    if (it == ctx->function_profiles.end()) {
        ctx->last_error = "Function not found";
        return -1;
    }

    FunctionProfile& profile = it->second;

    // Auto-optimization logic based on call count
    if (profile.call_count >= JIT_PROFILE_THRESHOLD) {
        JITOptLevel new_level = profile.current_opt_level;

        // Progressive optimization
        if (profile.call_count >= JIT_PROFILE_THRESHOLD * 10 &&
            profile.current_opt_level < JIT_OPT_AGGRESSIVE) {
            new_level = JIT_OPT_AGGRESSIVE;
        } else if (profile.call_count >= JIT_PROFILE_THRESHOLD &&
                   profile.current_opt_level < JIT_OPT_BASIC) {
            new_level = JIT_OPT_BASIC;
        }

        // Only recompile if optimization level changed
        if (new_level != profile.current_opt_level) {
            return jit_recompile_function(ctx, name, new_level);
        }
    }

    return 0; // No reoptimization needed
}

} // extern "C"