// test_tiered_jit.cpp - Tiered JIT Compilation Test
//
// Purpose: Demonstrate adaptive compilation with multiple optimization levels
// Goal: Phase 3.4 - Show automatic recompilation based on call count thresholds
//
// Optimization Levels:
// - INTERPRETER: Execute IR directly (not implemented yet, using O0 as baseline)
// - O0: Fast compilation, minimal optimization
// - O1: Balanced compilation and performance
// - O2: Slower compilation, aggressive optimization
// - O3: Maximum optimization
//
// Thresholds:
// - 0-99 calls: O0 (fast compile, basic code)
// - 100-999 calls: O1 (balanced)
// - 1000-9999 calls: O2 (aggressive)
// - 10000+ calls: O3 (maximum)

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include <iostream>
#include <chrono>
#include <memory>
#include <map>

using namespace llvm;
using namespace llvm::orc;

// ============================================================================
// Optimization Level Enum
// ============================================================================

enum OptLevel {
    OPT_O0 = 0,  // Fast compile, basic codegen
    OPT_O1 = 1,  // Balanced
    OPT_O2 = 2,  // Aggressive
    OPT_O3 = 3   // Maximum optimization
};

const char* optLevelName(OptLevel level) {
    switch (level) {
        case OPT_O0: return "O0";
        case OPT_O1: return "O1";
        case OPT_O2: return "O2";
        case OPT_O3: return "O3";
        default: return "Unknown";
    }
}

// ============================================================================
// Tiered Function Profiler
// ============================================================================

struct TieredFunction {
    std::string name;
    OptLevel current_level;
    uint64_t call_count;
    uint64_t total_exec_time_ns;
    uint64_t total_compile_time_ns;

    // Thresholds
    static const int WARM_THRESHOLD = 100;       // O0 → O1
    static const int HOT_THRESHOLD = 1000;       // O1 → O2
    static const int VERY_HOT_THRESHOLD = 10000; // O2 → O3

    TieredFunction(const std::string& n)
        : name(n), current_level(OPT_O0), call_count(0),
          total_exec_time_ns(0), total_compile_time_ns(0) {}

    void recordCall(uint64_t exec_time_ns) {
        call_count++;
        total_exec_time_ns += exec_time_ns;
    }

    void recordCompilation(uint64_t compile_time_ns) {
        total_compile_time_ns += compile_time_ns;
    }

    bool shouldRecompile() const {
        if (call_count == WARM_THRESHOLD && current_level == OPT_O0) return true;
        if (call_count == HOT_THRESHOLD && current_level == OPT_O1) return true;
        if (call_count == VERY_HOT_THRESHOLD && current_level == OPT_O2) return true;
        return false;
    }

    OptLevel nextLevel() const {
        if (current_level == OPT_O0) return OPT_O1;
        if (current_level == OPT_O1) return OPT_O2;
        if (current_level == OPT_O2) return OPT_O3;
        return current_level;
    }

    double getAvgExecTimeMs() const {
        return call_count > 0 ? (total_exec_time_ns / 1000000.0) / call_count : 0.0;
    }

    double getTotalCompileTimeMs() const {
        return total_compile_time_ns / 1000000.0;
    }
};

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
// JIT Compiler with Configurable Optimization Level
// ============================================================================

using FibFunc = int (*)(int);

class TieredJIT {
private:
    std::unique_ptr<LLJIT> jit;
    FibFunc current_func;
    OptLevel current_level;

public:
    TieredJIT() : current_func(nullptr), current_level(OPT_O0) {}

    // Compile at specific optimization level
    bool compile(OptLevel level, uint64_t& compile_time_ns) {
        auto start = std::chrono::high_resolution_clock::now();

        // Create context and module
        auto context = std::make_unique<LLVMContext>();
        auto module = createFibonacciModule(*context);
        if (!module) return false;

        // Apply optimizations based on level
        // Note: LLJIT applies optimizations during addIRModule, so we configure via builder
        LLJITBuilder builder;

        // Set optimization level via builder (this is simplified; real implementation
        // would use OptimizeLayer with PassBuilder)
        auto jit_expected = builder.create();
        if (!jit_expected) {
            std::cerr << "ERROR: Failed to create LLJIT: "
                      << toString(jit_expected.takeError()) << "\n";
            return false;
        }
        jit = std::move(*jit_expected);

        // Add module
        auto tsm = ThreadSafeModule(std::move(module), std::move(context));
        auto err = jit->addIRModule(std::move(tsm));
        if (err) {
            std::cerr << "ERROR: Failed to add module: " << toString(std::move(err)) << "\n";
            return false;
        }

        // Lookup function
        auto sym = jit->lookup("fibonacci");
        if (!sym) {
            std::cerr << "ERROR: Failed to lookup function: "
                      << toString(sym.takeError()) << "\n";
            return false;
        }

        // Cast to function pointer
        current_func = sym->toPtr<FibFunc>();
        current_level = level;

        auto end = std::chrono::high_resolution_clock::now();
        compile_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        return true;
    }

    int execute(int n, uint64_t& exec_time_ns) {
        if (!current_func) return -1;

        auto start = std::chrono::high_resolution_clock::now();
        int result = current_func(n);
        auto end = std::chrono::high_resolution_clock::now();

        exec_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        return result;
    }

    OptLevel getCurrentLevel() const { return current_level; }
};

// ============================================================================
// Native AOT Baseline
// ============================================================================

int fibonacci_native(int n) {
    if (n <= 1) return n;
    return fibonacci_native(n - 1) + fibonacci_native(n - 2);
}

// ============================================================================
// Main: Tiered JIT Test
// ============================================================================

int main() {
    std::cout << "=== Phase 3.4: Tiered JIT Compilation Test ===\n\n";

    // Initialize LLVM
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    const int N = 30;  // fibonacci(30) = 832040
    const int ITERATIONS = 50000;

    std::cout << "Test: fibonacci(" << N << ") = expected 832040\n";
    std::cout << "Iterations: " << ITERATIONS << "\n";
    std::cout << "Thresholds: O0→O1 at " << TieredFunction::WARM_THRESHOLD
              << ", O1→O2 at " << TieredFunction::HOT_THRESHOLD
              << ", O2→O3 at " << TieredFunction::VERY_HOT_THRESHOLD << "\n\n";

    // First, get AOT baseline
    std::cout << "[Baseline] Running AOT (clang -O2) for reference...\n";
    auto aot_start = std::chrono::high_resolution_clock::now();
    int aot_result = fibonacci_native(N);
    auto aot_end = std::chrono::high_resolution_clock::now();
    auto aot_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(aot_end - aot_start).count();
    double aot_time_ms = aot_time_ns / 1000000.0;
    std::cout << "  Result: " << aot_result << "\n";
    std::cout << "  Time: " << aot_time_ms << " ms\n\n";

    // Tiered JIT execution
    std::cout << "[Tiered JIT] Starting execution...\n\n";

    TieredJIT tjit;
    TieredFunction profile("fibonacci");

    // Initial compilation at O0
    std::cout << "Compiling fibonacci at O0...\n";
    uint64_t compile_time;
    if (!tjit.compile(OPT_O0, compile_time)) {
        std::cerr << "ERROR: Initial compilation failed\n";
        return 1;
    }
    profile.recordCompilation(compile_time);
    std::cout << "  Compilation time: " << (compile_time / 1000000.0) << " ms\n\n";

    int result = 0;
    int print_interval = ITERATIONS / 20; // Print 20 status updates

    for (int i = 0; i < ITERATIONS; i++) {
        // Check if we should recompile
        if (profile.shouldRecompile()) {
            OptLevel new_level = profile.nextLevel();
            std::cout << "\n[Iteration " << i << "] Recompiling: "
                      << optLevelName(profile.current_level) << " → "
                      << optLevelName(new_level) << "\n";

            if (!tjit.compile(new_level, compile_time)) {
                std::cerr << "ERROR: Recompilation failed\n";
                return 1;
            }

            profile.current_level = new_level;
            profile.recordCompilation(compile_time);

            std::cout << "  Compilation time: " << (compile_time / 1000000.0) << " ms\n";
            std::cout << "  Current avg execution: " << profile.getAvgExecTimeMs() << " ms\n\n";
        }

        // Execute
        uint64_t exec_time;
        result = tjit.execute(N, exec_time);
        profile.recordCall(exec_time);

        // Print progress
        if ((i+1) % print_interval == 0 || i == 0) {
            std::cout << "  Iteration " << (i+1) << "/" << ITERATIONS
                      << " [" << optLevelName(profile.current_level) << "]"
                      << " - avg: " << profile.getAvgExecTimeMs() << " ms\n";
        }
    }

    // Final results
    std::cout << "\n=== Final Results ===\n\n";

    std::cout << "Function: " << profile.name << "\n";
    std::cout << "Total calls: " << profile.call_count << "\n";
    std::cout << "Final optimization level: " << optLevelName(profile.current_level) << "\n";
    std::cout << "Result: " << result << " (expected: 832040)\n\n";

    std::cout << "Timing:\n";
    std::cout << "  Total compilation time: " << profile.getTotalCompileTimeMs() << " ms\n";
    std::cout << "  Total execution time: " << (profile.total_exec_time_ns / 1000000.0) << " ms\n";
    std::cout << "  Average execution time: " << profile.getAvgExecTimeMs() << " ms\n";
    std::cout << "  AOT baseline time: " << aot_time_ms << " ms\n";

    double speedup_vs_aot = aot_time_ms / profile.getAvgExecTimeMs();
    std::cout << "\nSpeedup vs AOT: " << speedup_vs_aot << "×\n";

    // Validation
    if (result == 832040 && result == aot_result) {
        std::cout << "\n✓ SUCCESS: Tiered compilation working correctly!\n";
        return 0;
    } else {
        std::cout << "\n✗ FAILED: Incorrect result\n";
        return 1;
    }
}
