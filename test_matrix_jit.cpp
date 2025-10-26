// test_matrix_jit.cpp - Matrix Multiply JIT Test
//
// Purpose: Demonstrate tiered JIT with computation-heavy workload
// Goal: Show that O3 optimization makes a REAL difference on loops
//
// This should show bigger performance gains than fibonacci!

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

#include <iostream>
#include <chrono>
#include <memory>
#include <cstring>

using namespace llvm;
using namespace llvm::orc;

// ============================================================================
// IR Generation: Create matrix multiply function
// ============================================================================

std::unique_ptr<Module> createMatrixModule(LLVMContext& ctx) {
    auto mod = std::make_unique<Module>("matrix_module", ctx);

    // void matrix_multiply(int* A, int* B, int* C, int N)
    // C[i][j] = sum(A[i][k] * B[k][j])
    auto* i32Type = Type::getInt32Ty(ctx);
    auto* ptrType = PointerType::get(i32Type, 0);

    FunctionType* matMulType = FunctionType::get(
        Type::getVoidTy(ctx),
        {ptrType, ptrType, ptrType, i32Type},
        false
    );

    Function* matMulFunc = Function::Create(
        matMulType,
        Function::ExternalLinkage,
        "matrix_multiply",
        mod.get()
    );

    // Get parameters
    auto args = matMulFunc->arg_begin();
    Value* A = &*args++;
    Value* B = &*args++;
    Value* C = &*args++;
    Value* N = &*args++;

    A->setName("A");
    B->setName("B");
    C->setName("C");
    N->setName("N");

    // Create basic blocks
    BasicBlock* entry = BasicBlock::Create(ctx, "entry", matMulFunc);
    BasicBlock* loop_i = BasicBlock::Create(ctx, "loop_i", matMulFunc);
    BasicBlock* loop_j = BasicBlock::Create(ctx, "loop_j", matMulFunc);
    BasicBlock* loop_k = BasicBlock::Create(ctx, "loop_k", matMulFunc);
    BasicBlock* loop_k_body = BasicBlock::Create(ctx, "loop_k_body", matMulFunc);
    BasicBlock* loop_k_end = BasicBlock::Create(ctx, "loop_k_end", matMulFunc);
    BasicBlock* loop_j_end = BasicBlock::Create(ctx, "loop_j_end", matMulFunc);
    BasicBlock* loop_i_end = BasicBlock::Create(ctx, "loop_i_end", matMulFunc);
    BasicBlock* ret = BasicBlock::Create(ctx, "return", matMulFunc);

    IRBuilder<> builder(entry);

    // Entry: i = 0
    builder.CreateBr(loop_i);

    // Loop i
    builder.SetInsertPoint(loop_i);
    PHINode* i = builder.CreatePHI(i32Type, 2, "i");
    i->addIncoming(ConstantInt::get(i32Type, 0), entry);

    Value* i_cond = builder.CreateICmpSLT(i, N, "i_cond");
    builder.CreateCondBr(i_cond, loop_j, ret);

    // Loop j
    builder.SetInsertPoint(loop_j);
    PHINode* j = builder.CreatePHI(i32Type, 2, "j");
    j->addIncoming(ConstantInt::get(i32Type, 0), loop_i);

    Value* j_cond = builder.CreateICmpSLT(j, N, "j_cond");
    builder.CreateCondBr(j_cond, loop_k, loop_i_end);

    // Loop k (initialize sum = 0)
    builder.SetInsertPoint(loop_k);
    PHINode* k = builder.CreatePHI(i32Type, 2, "k");
    PHINode* sum = builder.CreatePHI(i32Type, 2, "sum");
    k->addIncoming(ConstantInt::get(i32Type, 0), loop_j);
    sum->addIncoming(ConstantInt::get(i32Type, 0), loop_j);

    Value* k_cond = builder.CreateICmpSLT(k, N, "k_cond");
    builder.CreateCondBr(k_cond, loop_k_body, loop_k_end);

    // Loop k body: sum += A[i][k] * B[k][j]
    builder.SetInsertPoint(loop_k_body);

    // A_idx = i * N + k
    Value* i_mul_N = builder.CreateMul(i, N, "i_mul_N");
    Value* A_idx = builder.CreateAdd(i_mul_N, k, "A_idx");
    Value* A_ptr = builder.CreateGEP(i32Type, A, A_idx, "A_ptr");
    Value* A_val = builder.CreateLoad(i32Type, A_ptr, "A_val");

    // B_idx = k * N + j
    Value* k_mul_N = builder.CreateMul(k, N, "k_mul_N");
    Value* B_idx = builder.CreateAdd(k_mul_N, j, "B_idx");
    Value* B_ptr = builder.CreateGEP(i32Type, B, B_idx, "B_ptr");
    Value* B_val = builder.CreateLoad(i32Type, B_ptr, "B_val");

    // sum += A_val * B_val
    Value* prod = builder.CreateMul(A_val, B_val, "prod");
    Value* new_sum = builder.CreateAdd(sum, prod, "new_sum");

    Value* k_inc = builder.CreateAdd(k, ConstantInt::get(i32Type, 1), "k_inc");
    k->addIncoming(k_inc, loop_k_body);
    sum->addIncoming(new_sum, loop_k_body);

    builder.CreateBr(loop_k);

    // Loop k end: store C[i][j] = sum
    builder.SetInsertPoint(loop_k_end);
    Value* C_idx = builder.CreateAdd(i_mul_N, j, "C_idx");
    Value* C_ptr = builder.CreateGEP(i32Type, C, C_idx, "C_ptr");
    builder.CreateStore(sum, C_ptr);

    Value* j_inc = builder.CreateAdd(j, ConstantInt::get(i32Type, 1), "j_inc");
    j->addIncoming(j_inc, loop_k_end);

    builder.CreateBr(loop_j);

    // Loop j end
    builder.SetInsertPoint(loop_j_end);
    builder.CreateBr(loop_i_end);

    // Loop i end
    builder.SetInsertPoint(loop_i_end);
    Value* i_inc = builder.CreateAdd(i, ConstantInt::get(i32Type, 1), "i_inc");
    i->addIncoming(i_inc, loop_j);

    builder.CreateBr(loop_i);

    // Return
    builder.SetInsertPoint(ret);
    builder.CreateRetVoid();

    // Verify
    if (verifyFunction(*matMulFunc, &errs())) {
        std::cerr << "ERROR: Matrix multiply function verification failed\n";
        return nullptr;
    }

    return mod;
}

// ============================================================================
// JIT Compiler
// ============================================================================

using MatMulFunc = void (*)(int*, int*, int*, int);

MatMulFunc compileMatrixMultiply() {
    auto context = std::make_unique<LLVMContext>();
    auto module = createMatrixModule(*context);
    if (!module) return nullptr;

    // Create LLJIT
    auto jit_expected = LLJITBuilder().create();
    if (!jit_expected) {
        std::cerr << "ERROR: Failed to create LLJIT: "
                  << toString(jit_expected.takeError()) << "\n";
        return nullptr;
    }
    auto jit = std::move(*jit_expected);

    // Add module
    auto tsm = ThreadSafeModule(std::move(module), std::move(context));
    auto err = jit->addIRModule(std::move(tsm));
    if (err) {
        std::cerr << "ERROR: Failed to add module: " << toString(std::move(err)) << "\n";
        return nullptr;
    }

    // Lookup function
    auto sym = jit->lookup("matrix_multiply");
    if (!sym) {
        std::cerr << "ERROR: Failed to lookup function: "
                  << toString(sym.takeError()) << "\n";
        return nullptr;
    }

    return sym->toPtr<MatMulFunc>();
}

// ============================================================================
// Native baseline
// ============================================================================

void matrix_multiply_native(int* A, int* B, int* C, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int sum = 0;
            for (int k = 0; k < N; k++) {
                sum += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// ============================================================================
// Test
// ============================================================================

int main() {
    std::cout << "=== Matrix Multiply JIT Test ===\n\n";

    // Initialize LLVM
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    const int N = 64;  // 64x64 matrices
    const int SIZE = N * N;

    // Allocate matrices
    int* A = new int[SIZE];
    int* B = new int[SIZE];
    int* C_native = new int[SIZE];
    int* C_jit = new int[SIZE];

    // Initialize matrices
    for (int i = 0; i < SIZE; i++) {
        A[i] = i % 10;
        B[i] = (i * 2) % 10;
    }

    std::cout << "Matrix size: " << N << "x" << N << "\n";
    std::cout << "Total elements: " << SIZE << "\n\n";

    // Test 1: Native AOT
    std::cout << "[1/2] Native (clang -O2)...\n";
    auto start_native = std::chrono::high_resolution_clock::now();
    matrix_multiply_native(A, B, C_native, N);
    auto end_native = std::chrono::high_resolution_clock::now();
    auto time_native = std::chrono::duration_cast<std::chrono::microseconds>(
        end_native - start_native).count();

    std::cout << "  Time: " << (time_native / 1000.0) << " ms\n";
    std::cout << "  C[0][0] = " << C_native[0] << "\n";
    std::cout << "  C[N-1][N-1] = " << C_native[SIZE-1] << "\n\n";

    // Test 2: JIT
    std::cout << "[2/2] JIT (LLVM)...\n";
    auto matmul_jit = compileMatrixMultiply();
    if (!matmul_jit) {
        std::cerr << "ERROR: Failed to compile JIT function\n";
        return 1;
    }

    auto start_jit = std::chrono::high_resolution_clock::now();
    matmul_jit(A, B, C_jit, N);
    auto end_jit = std::chrono::high_resolution_clock::now();
    auto time_jit = std::chrono::duration_cast<std::chrono::microseconds>(
        end_jit - start_jit).count();

    std::cout << "  Time: " << (time_jit / 1000.0) << " ms\n";
    std::cout << "  C[0][0] = " << C_jit[0] << "\n";
    std::cout << "  C[N-1][N-1] = " << C_jit[SIZE-1] << "\n\n";

    // Verify results match
    bool match = true;
    for (int i = 0; i < SIZE; i++) {
        if (C_native[i] != C_jit[i]) {
            std::cout << "MISMATCH at index " << i << ": native=" << C_native[i]
                      << ", jit=" << C_jit[i] << "\n";
            match = false;
            break;
        }
    }

    // Results
    std::cout << "=== Results ===\n\n";
    std::cout << "Native time: " << (time_native / 1000.0) << " ms\n";
    std::cout << "JIT time:    " << (time_jit / 1000.0) << " ms\n";

    double speedup = (double)time_native / time_jit;
    std::cout << "\nJIT vs Native: " << speedup << "×\n";

    if (match) {
        std::cout << "\n✓ SUCCESS: Results match!\n";
        delete[] A;
        delete[] B;
        delete[] C_native;
        delete[] C_jit;
        return 0;
    } else {
        std::cout << "\n✗ FAILED: Results don't match\n";
        delete[] A;
        delete[] B;
        delete[] C_native;
        delete[] C_jit;
        return 1;
    }
}
