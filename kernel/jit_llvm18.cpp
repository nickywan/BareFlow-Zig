// jit_llvm18.cpp - LLVM 18 implementation of JIT interface

#include "jit_interface.h"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>

#include <memory>
#include <string>

using namespace llvm;
using namespace llvm::orc;

struct JITContext {
    std::unique_ptr<LLJIT> jit;
    std::unique_ptr<LLVMContext> context;
    std::string last_error;
    JITStats stats;
    
    JITContext() : stats{0, 0, 0} {
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
    
    return sym->toPtr<void*>();
}

int jit_recompile_function(JITContext* ctx, const char* name, JITOptLevel opt) {
    // TODO: Phase 1b - Implement recompilation with different opt levels
    ctx->last_error = "Recompilation not yet implemented";
    return -1;
}

void jit_get_stats(JITContext* ctx, JITStats* stats) {
    if (ctx && stats) {
        *stats = ctx->stats;
    }
}

const char* jit_get_last_error(JITContext* ctx) {
    return ctx ? ctx->last_error.c_str() : "Invalid context";
}

} // extern "C"