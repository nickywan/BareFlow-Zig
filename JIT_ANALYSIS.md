# JIT Runtime Analysis - Port to Bare-Metal

**Date**: 2025-10-26
**Branch**: feat/true-jit-unikernel
**Goal**: Port `kernel/jit_llvm18.cpp` to bare-metal for true self-optimizing unikernel

---

## 📊 Current Implementation Analysis

### File: `kernel/jit_llvm18.cpp` (282 lines)

**Purpose**: LLVM 18 OrcJIT wrapper with profiling and adaptive optimization

**Interface**: `kernel/jit_interface.h` (C API for kernel integration)

---

## 🔗 LLVM Dependencies

### Core LLVM Headers Required:
```cpp
#include <llvm/ExecutionEngine/Orc/LLJIT.h>          // Main JIT engine
#include <llvm/IR/LLVMContext.h>                     // IR context
#include <llvm/IR/Module.h>                          // Module representation
#include <llvm/IRReader/IRReader.h>                  // Parse IR from file
#include <llvm/Support/SourceMgr.h>                  // Error diagnostics
#include <llvm/Support/TargetSelect.h>               // Native target init
#include <llvm/Support/MemoryBuffer.h>               // Memory IR loading
```

### Key LLVM Classes Used:
- **LLJIT**: Main JIT execution engine (Orc v2)
- **LLVMContext**: IR parsing and module context
- **ThreadSafeModule**: Multi-threaded module wrapper
- **MemoryBuffer**: In-memory bitcode loading
- **SMDiagnostic**: Error reporting

### LLVM Initialization Required:
```cpp
InitializeNativeTarget();           // x86 codegen
InitializeNativeTargetAsmPrinter(); // Assembly output
InitializeNativeTargetAsmParser();  // Assembly parsing
```

---

## 📦 C++ Standard Library Dependencies

### Containers & Smart Pointers:
```cpp
std::unique_ptr<LLJIT>                              // JIT engine ownership
std::unique_ptr<LLVMContext>                        // Context ownership
std::unordered_map<std::string, FunctionProfile>   // Function tracking
std::string                                         // Error messages, names
std::vector                                         // (in struct definitions)
```

### Memory Management:
- `std::make_unique<T>()` - Smart pointer factory
- `std::move()` - Move semantics
- `new` / `delete` - Heap allocation

### Standard Headers:
```cpp
#include <memory>        // unique_ptr, make_unique
#include <string>        // std::string
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector
#include <cstring>       // strcmp, etc.
```

---

## 🎯 API Functions (11 total)

### Lifecycle:
1. `jit_create()` - Create JIT context
2. `jit_destroy()` - Cleanup

### Module Loading:
3. `jit_load_bitcode(path)` - Load from file
4. `jit_load_bitcode_memory(data, size)` - Load from memory
5. `jit_unload_module()` - Unload module

### Function Management:
6. `jit_find_function(name)` - Lookup function pointer
7. `jit_recompile_function(name, opt_level)` - Reoptimize
8. `jit_get_function_info(name, info)` - Get profiling data
9. `jit_list_functions(infos, max)` - List all functions

### Profiling:
10. `jit_record_call(name, cycles)` - Record execution
11. `jit_auto_optimize(name)` - Auto-reoptimize after 100 calls

### Utilities:
12. `jit_get_stats()` - Global stats
13. `jit_get_last_error()` - Error message

---

## 💾 Data Structures

### JITContext (per-instance state):
```cpp
struct JITContext {
    std::unique_ptr<LLJIT> jit;                              // ~50KB+
    std::unique_ptr<LLVMContext> context;                    // ~10KB+
    std::string last_error;                                  // Variable
    JITStats stats;                                          // 40 bytes
    std::unordered_map<std::string, FunctionProfile> profiles; // ~N*200 bytes
};
```

### FunctionProfile (per-function):
```cpp
struct FunctionProfile {
    std::string name;               // ~32 bytes
    void* code_ptr;                 // 4 bytes
    uint64_t call_count;            // 8 bytes
    uint64_t total_cycles;          // 8 bytes
    uint32_t code_size;             // 4 bytes
    JITOptLevel current_opt_level;  // 4 bytes
};
// Total: ~60 bytes + string overhead
```

---

## ⚠️ Bare-Metal Challenges

### 1. C++ Standard Library
**Problem**: No `std::string`, `std::unordered_map`, `std::unique_ptr`
**Solution**:
- Replace `std::string` → `char[64]` (fixed-size)
- Replace `std::unordered_map` → Custom hash table or linear array
- Replace `std::unique_ptr` → Raw pointers with manual cleanup
- Implement `operator new/delete` → Custom allocator

### 2. LLVM Dependencies
**Problem**: LLVM requires C++ runtime, exceptions, RTTI
**Solution**:
- Build LLVM with `-DLLVM_ENABLE_RTTI=OFF -DLLVM_ENABLE_EH=OFF`
- Static link LLVM libs (~10-50MB compressed)
- Provide custom allocator for LLVM (`operator new/delete`)

### 3. Memory Footprint
**Problem**: LLVM JIT needs significant heap space
**Estimated Requirements**:
- LLVM JIT engine: ~50-100KB baseline
- IR parsing: ~10-50KB per module
- Code generation: ~5-20KB per function
- Working memory: ~1-2MB recommended

**Total Estimate**: **2-5MB heap minimum**

### 4. File I/O
**Problem**: `jit_load_bitcode(path)` needs filesystem
**Solution**:
- Use `jit_load_bitcode_memory()` exclusively
- Embed bitcode in binary or load from FAT16
- Remove file-based API for bare-metal

### 5. Threading
**Problem**: `ThreadSafeModule` assumes threading support
**Solution**:
- Single-threaded mode only
- Remove `ThreadSafeModule` wrapper (use raw `Module*`)
- No multi-threaded compilation

---

## 🔧 Proposed Bare-Metal Design

### Minimal API (8 functions):
```c
// Lifecycle
jit_context_t* jit_init(void* heap, size_t heap_size);
void jit_shutdown(jit_context_t* ctx);

// Module loading (memory only)
int jit_load_ir(jit_context_t* ctx, const uint8_t* ir_data, size_t size);

// Function lookup
void* jit_get_function(jit_context_t* ctx, const char* name);

// Recompilation
int jit_recompile(jit_context_t* ctx, const char* name, int opt_level);

// Profiling
void jit_profile_call(jit_context_t* ctx, const char* name, uint64_t cycles);
int jit_should_reoptimize(jit_context_t* ctx, const char* name);

// Stats
void jit_get_stats(jit_context_t* ctx, jit_stats_t* stats);
```

### Simplified Data Structures:
```c
#define JIT_MAX_FUNCTIONS 32
#define JIT_NAME_MAX 32

typedef struct {
    char name[JIT_NAME_MAX];
    void* code_ptr;
    uint64_t call_count;
    uint64_t total_cycles;
    uint8_t opt_level;
} jit_function_t;

typedef struct {
    void* llvm_jit;          // Opaque LLVM handle
    void* llvm_context;      // Opaque LLVM context
    char last_error[128];
    jit_function_t functions[JIT_MAX_FUNCTIONS];
    int function_count;
    uint64_t total_compiles;
} jit_context_t;
```

---

## 📏 Memory Budget

### Conservative Estimate:
```
LLVM static libs:  ~500KB (stripped, MinSizeRel)
JIT runtime:       ~100KB (baseline)
Heap (working):    ~2MB (recommended)
Code cache:        ~500KB (generated code)
-------------------------------------------
Total:             ~3.1MB
```

### Target for Unikernel:
- Binary size: <500KB (LLVM + app)
- Runtime heap: 2MB dedicated
- Total footprint: ~2.5MB

---

## ✅ Next Steps

### Phase 3.1 - LLVM Bare-Metal Build (Week 3)
1. Build LLVM 18 with bare-metal flags:
   ```bash
   cmake -DLLVM_TARGETS_TO_BUILD=X86 \
         -DLLVM_ENABLE_RTTI=OFF \
         -DLLVM_ENABLE_EH=OFF \
         -DCMAKE_BUILD_TYPE=MinSizeRel \
         -DLLVM_BUILD_TOOLS=OFF \
         -DLLVM_INCLUDE_TESTS=OFF
   ```

2. Test static linking in userspace first
3. Measure actual binary size

### Phase 3.2 - Custom Allocator (Week 3)
1. Implement `jit_heap_init(ptr, size)`
2. Override `operator new/delete` for LLVM
3. Test with simple module (fibonacci)

### Phase 3.3 - Minimal JIT Port (Week 4)
1. Create `kernel_lib/jit/runtime_llvm.cpp`
2. Port core functions: init, load_ir, get_function
3. Test in bare-metal with embedded IR

### Phase 3.4 - Integration (Week 4-5)
1. Integrate with tinyllama/main.c
2. Measure overhead vs AOT baseline
3. Compare recompilation speedup

---

## 🎯 Success Criteria

- ✅ LLVM static libs < 500KB
- ✅ JIT init() works bare-metal
- ✅ Load IR from memory
- ✅ Compile and call fibonacci
- ✅ Recompile with -O2 after 100 calls
- ✅ Measure >20% speedup vs AOT
- ✅ Total overhead <10%

---

**Created**: 2025-10-26
**For**: Phase 3 JIT Runtime Implementation
