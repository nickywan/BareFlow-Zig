// test_llvm_interpreter.cpp - LLVM Interpreter Mode Test
//
// Purpose: Test LLVM IR interpretation (slow) vs JIT compilation (fast)
// Goal: Demonstrate tiered execution for Phase 3.3
//
// Execution modes:
// 1. Interpreter: Execute IR directly (slow, no compilation)
// 2. JIT O0: Fast compilation, unoptimized code
// 3. JIT O2: Slower compilation, optimized code

#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

#include <iostream>
#include <chrono>
#include <memory>

using namespace llvm;
using namespace llvm::orc;

// ============================================================================
// IR Generation: Create fibonacci function
// ============================================================================

std::unique_ptr<Module> createFibonacciModule(LLVMContext& ctx) {
    auto mod = std::make_unique<Module>("fib_module", ctx);

    // int fibonacci(int n)
    auto* i32Type = Type::getInt32Ty(ctx);
    FunctionType* fibType = FunctionType::get(i32Type, {i32Type}, false);
    Function* fibFunc = Function::Create(fibType, Function::ExternalLinkage, "fibonacci", mod.get());

    // Entry block
    BasicBlock* entry = BasicBlock::Create(ctx, "entry", fibFunc);
    BasicBlock* baseCase = BasicBlock::Create(ctx, "base_case", fibFunc);
    BasicBlock* recursiveCase = BasicBlock::Create(ctx, "recursive", fibFunc);
    BasicBlock* ret = BasicBlock::Create(ctx, "return", fibFunc);

    IRBuilder<> builder(entry);
    Value* n = fibFunc->arg_begin();

    // if (n <= 1) goto base_case else goto recursive
    Value* cond = builder.CreateICmpSLE(n, ConstantInt::get(i32Type, 1), "cond");
    builder.CreateCondBr(cond, baseCase, recursiveCase);

    // base_case: return n
    builder.SetInsertPoint(baseCase);
    builder.CreateBr(ret);

    // recursive: return fib(n-1) + fib(n-2)
    builder.SetInsertPoint(recursiveCase);
    Value* n_minus_1 = builder.CreateSub(n, ConstantInt::get(i32Type, 1), "n_minus_1");
    Value* n_minus_2 = builder.CreateSub(n, ConstantInt::get(i32Type, 2), "n_minus_2");

    Value* fib1 = builder.CreateCall(fibFunc, {n_minus_1}, "fib1");
    Value* fib2 = builder.CreateCall(fibFunc, {n_minus_2}, "fib2");
    Value* result = builder.CreateAdd(fib1, fib2, "result");
    builder.CreateBr(ret);

    // return block
    builder.SetInsertPoint(ret);
    PHINode* phi = builder.CreatePHI(i32Type, 2, "retval");
    phi->addIncoming(n, baseCase);
    phi->addIncoming(result, recursiveCase);
    builder.CreateRet(phi);

    // Verify
    if (verifyFunction(*fibFunc, &errs())) {
        std::cerr << "ERROR: Fibonacci function verification failed\n";
        return nullptr;
    }

    return mod;
}

// ============================================================================
// Profiler: Track function calls
// ============================================================================

struct FunctionProfile {
    std::string name;
    uint64_t call_count = 0;
    uint64_t total_time_ns = 0;

    void recordCall(uint64_t time_ns) {
        call_count++;
        total_time_ns += time_ns;
    }

    double getAvgTimeMs() const {
        return call_count > 0 ? (total_time_ns / 1000000.0) / call_count : 0.0;
    }
};

// ============================================================================
// Test 1: LLVM Interpreter (interpreted execution)
// ============================================================================

int testInterpreter(int n, FunctionProfile& profile) {
    LLVMContext context;
    auto module = createFibonacciModule(context);
    if (!module) return -1;

    // Create Interpreter execution engine
    std::string errStr;
    EngineBuilder builder(std::move(module));
    builder.setEngineKind(EngineKind::Interpreter);
    builder.setErrorStr(&errStr);

    ExecutionEngine* engine = builder.create();
    if (!engine) {
        std::cerr << "ERROR: Failed to create Interpreter: " << errStr << "\n";
        return -1;
    }

    // Find function
    Function* fibFunc = engine->FindFunctionNamed("fibonacci");
    if (!fibFunc) {
        std::cerr << "ERROR: Function 'fibonacci' not found\n";
        return -1;
    }

    // Execute with timing
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<GenericValue> args(1);
    args[0].IntVal = APInt(32, n);
    GenericValue result = engine->runFunction(fibFunc, args);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    profile.recordCall(duration);

    delete engine;
    return result.IntVal.getSExtValue();
}

// ============================================================================
// Test 2: LLVM JIT Compilation (compiled execution)
// ============================================================================

int testJIT(int n, FunctionProfile& profile) {
    LLVMContext context;
    auto module = createFibonacciModule(context);
    if (!module) return -1;

    // Create LLJIT
    auto jit_expected = LLJITBuilder().create();
    if (!jit_expected) {
        std::cerr << "ERROR: Failed to create LLJIT: "
                  << toString(jit_expected.takeError()) << "\n";
        return -1;
    }
    auto jit = std::move(*jit_expected);

    // Add module (need to create new context for ThreadSafeModule)
    auto context_ptr = std::make_unique<LLVMContext>();
    auto tsm = ThreadSafeModule(std::move(module), std::move(context_ptr));
    auto err = jit->addIRModule(std::move(tsm));
    if (err) {
        std::cerr << "ERROR: Failed to add module: " << toString(std::move(err)) << "\n";
        return -1;
    }

    // Lookup function
    auto sym = jit->lookup("fibonacci");
    if (!sym) {
        std::cerr << "ERROR: Failed to lookup function: "
                  << toString(sym.takeError()) << "\n";
        return -1;
    }

    // Cast to function pointer
    using FibFunc = int (*)(int);
    auto fib_ptr = sym->toPtr<FibFunc>();

    // Execute with timing
    auto start = std::chrono::high_resolution_clock::now();
    int result = fib_ptr(n);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    profile.recordCall(duration);

    return result;
}

// ============================================================================
// Main: Compare Interpreter vs JIT
// ============================================================================

int main() {
    std::cout << "=== LLVM Interpreter vs JIT Comparison ===\n\n";

    // Initialize LLVM
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    const int N = 20;  // fibonacci(20) = 6765
    const int ITERATIONS = 10;

    FunctionProfile interpreterProfile;
    FunctionProfile jitProfile;

    interpreterProfile.name = "Interpreter";
    jitProfile.name = "JIT";

    std::cout << "Computing fibonacci(" << N << ") = expected 6765\n";
    std::cout << "Running " << ITERATIONS << " iterations each...\n\n";

    // Test Interpreter
    std::cout << "[1/2] Testing LLVM Interpreter (interpreted execution)...\n";
    int interpreterResult = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        interpreterResult = testInterpreter(N, interpreterProfile);
        std::cout << "  Iteration " << (i+1) << "/" << ITERATIONS
                  << ": fib(" << N << ") = " << interpreterResult << "\n";
    }

    // Test JIT
    std::cout << "\n[2/2] Testing LLVM JIT (compiled execution)...\n";
    int jitResult = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        jitResult = testJIT(N, jitProfile);
        std::cout << "  Iteration " << (i+1) << "/" << ITERATIONS
                  << ": fib(" << N << ") = " << jitResult << "\n";
    }

    // Results
    std::cout << "\n=== Results ===\n\n";

    std::cout << "Interpreter:\n";
    std::cout << "  Result: " << interpreterResult << "\n";
    std::cout << "  Calls: " << interpreterProfile.call_count << "\n";
    std::cout << "  Avg time: " << interpreterProfile.getAvgTimeMs() << " ms\n";
    std::cout << "  Total time: " << (interpreterProfile.total_time_ns / 1000000.0) << " ms\n";

    std::cout << "\nJIT:\n";
    std::cout << "  Result: " << jitResult << "\n";
    std::cout << "  Calls: " << jitProfile.call_count << "\n";
    std::cout << "  Avg time: " << jitProfile.getAvgTimeMs() << " ms\n";
    std::cout << "  Total time: " << (jitProfile.total_time_ns / 1000000.0) << " ms\n";

    // Speedup
    double speedup = interpreterProfile.getAvgTimeMs() / jitProfile.getAvgTimeMs();
    std::cout << "\n=== Performance ===\n";
    std::cout << "JIT is " << speedup << "× faster than Interpreter\n";

    // Validation
    bool success = (interpreterResult == jitResult) && (interpreterResult == 6765);
    if (success) {
        std::cout << "\n✓ SUCCESS: Both modes produced correct result (6765)\n";
        return 0;
    } else {
        std::cout << "\n✗ FAILED: Results don't match or incorrect\n";
        return 1;
    }
}
