#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <memory>

using namespace llvm;
using namespace llvm::orc;

int main(int argc, char** argv) {
    // Init LLVM
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    
    std::cout << "=== Fluid LLVM JIT Test ===\n\n";
    
    // Crée JIT
    auto JIT = LLJITBuilder().create();
    if (!JIT) {
        errs() << "Error creating JIT: " << toString(JIT.takeError()) << "\n";
        return 1;
    }
    std::cout << "[OK] LLVM JIT created\n";
    
    // Parse minimal.bc
    LLVMContext Context;
    SMDiagnostic Err;
    auto M = parseIRFile("libs/minimal.bc", Err, Context);
    if (!M) {
        Err.print(argv[0], errs());
        return 1;
    }
    std::cout << "[OK] Loaded minimal.bc (" << M->size() << " functions)\n";
    
    // Ajoute au JIT
    auto Err2 = (*JIT)->addIRModule(ThreadSafeModule(std::move(M), std::make_unique<LLVMContext>()));
    if (Err2) {
        errs() << "Error adding module: " << toString(std::move(Err2)) << "\n";
        return 1;
    }
    std::cout << "[OK] Module added to JIT\n";
    
    // Lookup strlen
    auto StrlenSym = (*JIT)->lookup("strlen");
    if (!StrlenSym) {
        errs() << "Error looking up strlen: " << toString(StrlenSym.takeError()) << "\n";
        return 1;
    }
    std::cout << "[OK] Found strlen function\n";
    
    // Cast to function pointer (LLVM 18 syntax)
    typedef size_t (*StrlenFunc)(const char*);
    auto strlen_jit = StrlenSym->toPtr<StrlenFunc>();  // ← FIX ICI
    
    // TEST!
    const char* test_str = "Fluid JIT is ALIVE!";
    size_t len = strlen_jit(test_str);
    
    std::cout << "\n[TEST] strlen(\"" << test_str << "\") = " << len << "\n";
    std::cout << "[EXPECTED] 19\n";
    
    if (len == 19) {
        std::cout << "\n✅ SUCCESS! LLVM JIT works!\n";
        return 0;
    } else {
        std::cout << "\n❌ FAILED! Expected 19, got " << len << "\n";
        return 1;
    }
}
