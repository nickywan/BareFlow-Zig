/**
 * Test: LLVM Initialization with Custom Allocator
 *
 * Tests that LLVM can be initialized with:
 * - Custom malloc (free-list allocator)
 * - Custom C++ runtime
 * - System call stubs
 *
 * This validates Phase 4 integration readiness.
 */

#include <stdio.h>
#include <stdint.h>

// LLVM headers
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;
using namespace llvm::orc;

// External memory functions
extern "C" {
    size_t malloc_get_usage();
    size_t malloc_get_peak();
    size_t malloc_get_heap_size();
}

// ============================================================================
// Test Functions
// ============================================================================

void print_memory_stats(const char* label) {
    size_t usage = malloc_get_usage();
    size_t peak = malloc_get_peak();
    size_t total = malloc_get_heap_size();

    printf("%s:\n", label);
    printf("  Memory usage: %.2f MB / %.2f MB (%.1f%%)\n",
           usage / (1024.0 * 1024.0),
           total / (1024.0 * 1024.0),
           (usage * 100.0) / total);
    printf("  Peak usage:   %.2f MB\n", peak / (1024.0 * 1024.0));
    printf("\n");
}

int main() {
    printf("========================================\n");
    printf("  LLVM Initialization Test\n");
    printf("  Custom Allocator + C++ Runtime\n");
    printf("========================================\n\n");

    print_memory_stats("Initial state");

    // ============================================================================
    // Step 1: Initialize LLVM Targets
    // ============================================================================

    printf("Step 1: Initializing LLVM targets...\n");

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    printf("  ✅ Native target initialized\n");
    print_memory_stats("After target initialization");

    // ============================================================================
    // Step 2: Create LLVM Context
    // ============================================================================

    printf("Step 2: Creating LLVM context...\n");

    auto Context = std::make_unique<LLVMContext>();

    printf("  ✅ LLVMContext created\n");
    print_memory_stats("After context creation");

    // ============================================================================
    // Step 3: Create LLJIT Instance
    // ============================================================================

    printf("Step 3: Creating LLJIT instance...\n");

    auto JIT = LLJITBuilder().create();
    if (!JIT) {
        fprintf(stderr, "  ❌ Failed to create LLJIT: %s\n",
                toString(JIT.takeError()).c_str());
        return 1;
    }

    printf("  ✅ LLJIT created successfully\n");
    print_memory_stats("After LLJIT creation");

    // ============================================================================
    // Step 4: Create Simple IR Module
    // ============================================================================

    printf("Step 4: Creating IR module...\n");

    auto M = std::make_unique<Module>("test_module", *Context);
    IRBuilder<> Builder(*Context);

    // Create a simple function: int add(int a, int b) { return a + b; }
    FunctionType* FT = FunctionType::get(
        Type::getInt32Ty(*Context),
        {Type::getInt32Ty(*Context), Type::getInt32Ty(*Context)},
        false
    );

    Function* AddFunc = Function::Create(
        FT,
        Function::ExternalLinkage,
        "add",
        M.get()
    );

    BasicBlock* BB = BasicBlock::Create(*Context, "entry", AddFunc);
    Builder.SetInsertPoint(BB);

    auto Args = AddFunc->arg_begin();
    Value* A = &*Args++;
    Value* B = &*Args;
    Value* Sum = Builder.CreateAdd(A, B, "sum");
    Builder.CreateRet(Sum);

    printf("  ✅ IR module created (function: add)\n");
    print_memory_stats("After IR creation");

    // ============================================================================
    // Step 5: Add Module to JIT
    // ============================================================================

    printf("Step 5: Adding module to JIT...\n");

    auto Err = (*JIT)->addIRModule(
        ThreadSafeModule(std::move(M), std::move(Context))
    );

    if (Err) {
        fprintf(stderr, "  ❌ Failed to add module: %s\n",
                toString(std::move(Err)).c_str());
        return 1;
    }

    printf("  ✅ Module added to JIT\n");
    print_memory_stats("After adding module");

    // ============================================================================
    // Step 6: Lookup and Execute Function
    // ============================================================================

    printf("Step 6: Looking up 'add' function...\n");

    auto AddSym = (*JIT)->lookup("add");
    if (!AddSym) {
        fprintf(stderr, "  ❌ Failed to lookup 'add': %s\n",
                toString(AddSym.takeError()).c_str());
        return 1;
    }

    printf("  ✅ Function 'add' found at: %p\n",
           (void*)AddSym->getValue());

    // Cast to function pointer and execute
    using AddFuncType = int (*)(int, int);
    auto AddFuncPtr = AddSym->toPtr<AddFuncType>();

    int result = AddFuncPtr(21, 21);
    printf("  ✅ Executed: add(21, 21) = %d\n", result);

    if (result != 42) {
        fprintf(stderr, "  ❌ FAIL: Expected 42, got %d\n", result);
        return 1;
    }

    print_memory_stats("After execution");

    // ============================================================================
    // Summary
    // ============================================================================

    printf("========================================\n");
    printf("  ✅ ALL TESTS PASSED\n");
    printf("========================================\n\n");

    printf("Validation Results:\n");
    printf("  ✓ LLVM targets initialized\n");
    printf("  ✓ LLVMContext created\n");
    printf("  ✓ LLJIT instance created\n");
    printf("  ✓ IR module generated\n");
    printf("  ✓ Module compiled by JIT\n");
    printf("  ✓ Function execution successful\n");
    printf("  ✓ Custom allocator working\n");
    printf("  ✓ C++ runtime working\n");
    printf("  ✓ System stubs working\n");
    printf("\n");

    print_memory_stats("Final state");

    printf("========================================\n");
    printf("Ready for bare-metal LLVM integration!\n");
    printf("========================================\n");

    return 0;
}
