// test_jit_minimal.cpp - Minimal LLVM JIT test for static linking verification
//
// Purpose: Test LLVM 18 OrcJIT with static linking to measure binary size
// Expected: Compile simple function, call it, measure total binary size

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>

#include <iostream>
#include <memory>

using namespace llvm;
using namespace llvm::orc;

// Create a simple IR module with "int add(int a, int b)"
std::unique_ptr<Module> createAddModule(LLVMContext& ctx) {
    auto mod = std::make_unique<Module>("test", ctx);

    // Create function type: int(int, int)
    auto* i32Type = Type::getInt32Ty(ctx);
    FunctionType* funcType = FunctionType::get(i32Type, {i32Type, i32Type}, false);

    // Create function
    Function* addFunc = Function::Create(funcType, Function::ExternalLinkage, "add", mod.get());

    // Create basic block
    BasicBlock* bb = BasicBlock::Create(ctx, "entry", addFunc);
    IRBuilder<> builder(bb);

    // Get arguments
    auto args = addFunc->arg_begin();
    Value* a = &*args;
    args++;
    Value* b = &*args;

    // Create: return a + b
    Value* result = builder.CreateAdd(a, b, "sum");
    builder.CreateRet(result);

    // Verify function
    if (verifyFunction(*addFunc, &errs())) {
        std::cerr << "Function verification failed\n";
        return nullptr;
    }

    return mod;
}

int main() {
    std::cout << "=== Minimal LLVM JIT Test ===\n\n";

    // Initialize LLVM
    std::cout << "[1/5] Initializing LLVM native target...\n";
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // Create JIT context
    std::cout << "[2/5] Creating LLJIT instance...\n";
    auto jit_expected = LLJITBuilder().create();
    if (!jit_expected) {
        std::cerr << "ERROR: Failed to create LLJIT: "
                  << toString(jit_expected.takeError()) << "\n";
        return 1;
    }
    auto jit = std::move(*jit_expected);

    // Create IR module
    std::cout << "[3/5] Creating IR module with add(int, int) function...\n";
    auto context = std::make_unique<LLVMContext>();
    auto module = createAddModule(*context);
    if (!module) {
        std::cerr << "ERROR: Failed to create module\n";
        return 1;
    }

    // Add module to JIT
    std::cout << "[4/5] Adding module to JIT...\n";
    auto tsm = ThreadSafeModule(std::move(module), std::move(context));
    auto err = jit->addIRModule(std::move(tsm));
    if (err) {
        std::cerr << "ERROR: Failed to add module: "
                  << toString(std::move(err)) << "\n";
        return 1;
    }

    // Lookup function
    std::cout << "[5/5] Looking up 'add' function...\n";
    auto sym = jit->lookup("add");
    if (!sym) {
        std::cerr << "ERROR: Failed to lookup function: "
                  << toString(sym.takeError()) << "\n";
        return 1;
    }

    // Cast to function pointer
    using AddFunc = int (*)(int, int);
    auto add_ptr = sym->toPtr<AddFunc>();

    // Test the function
    std::cout << "\n=== Testing JIT-compiled function ===\n";
    int a = 42, b = 58;
    int result = add_ptr(a, b);

    std::cout << "add(" << a << ", " << b << ") = " << result << "\n";

    if (result == 100) {
        std::cout << "\n✓ SUCCESS: JIT compilation and execution work!\n";
        return 0;
    } else {
        std::cerr << "\n✗ FAILED: Expected 100, got " << result << "\n";
        return 1;
    }
}
