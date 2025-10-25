# Runtime JIT Integration Plan

## 🎯 Goal: True Runtime Optimization

Replace offline PGO with **in-kernel JIT compilation** for on-the-fly optimization.

## Current State (Session 7)

### What Works
- ✅ Module system with profiling (rdtsc)
- ✅ Function profiler (call counts, cycle tracking)
- ✅ JIT allocator (CODE/DATA/METADATA pools)
- ✅ C++ runtime (new/delete)
- ✅ FAT16 filesystem (load .bc files from disk)
- ✅ LLVM 18 JIT interface (userspace prototype in kernel/jit_llvm18.cpp)

### What's Missing
- ❌ LLVM OrcJIT not integrated in kernel (C++ runtime issues)
- ❌ Modules are AOT-compiled native code (.mod), not bitcode (.bc)
- ❌ No runtime recompilation trigger
- ❌ No code pointer swapping

## Phase 1: Bitcode Module System (Week 1-2)

### Step 1.1: Bitcode Module Format
```c
// Replace .mod (native code) with .bc (LLVM bitcode)

typedef struct {
    uint32_t magic;         // "LLBC"
    char name[32];
    uint32_t bitcode_size;
    uint32_t entry_offset;  // Offset to entry function in bitcode
    uint8_t bitcode[];      // LLVM IR
} bitcode_module_t;
```

### Step 1.2: Module Compilation
```bash
# Old way (AOT):
clang -m32 -O2 -c module.c -o module.o
objcopy -O binary module.o module.mod

# New way (bitcode):
clang -m32 -emit-llvm -O0 -c module.c -o module.bc
python3 tools/wrap_bitcode.py module.bc > module_wrapped.bc
```

### Step 1.3: Bitcode Loader
```c
// kernel/bitcode_loader.c
int bitcode_module_load(const char* filename, bitcode_module_t** out);
```

## Phase 2: Minimal JIT (Week 3-4)

### Step 2.1: Thin JIT Wrapper
Instead of full LLVM OrcJIT, use **LLVM C API** (simpler, less runtime deps):

```c
// kernel/minimal_jit.h
typedef struct jit_engine_t jit_engine_t;

jit_engine_t* jit_create(void);
void* jit_compile(jit_engine_t* jit, const void* bitcode, size_t size,
                  const char* entry_name, int opt_level);
void jit_destroy(jit_engine_t* jit);
```

### Step 2.2: LLVM C API Integration
```c
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>

// Use LLVMCreateJITCompilerForModule (simpler than OrcJIT)
```

### Step 2.3: Static LLVM Linking
```makefile
# Link against static LLVM libraries
LLVM_LIBS = -lLLVMExecutionEngine -lLLVMMCJIT -lLLVMCore -lLLVMSupport
```

**Challenge**: LLVM C++ runtime needs:
- ~500KB code
- Memory allocator (jit_allocator ✅)
- Exception handling stubs (we have -fno-exceptions ✅)
- Thread-local storage (TLS) - need to implement

## Phase 3: Hot-Path Recompilation (Week 5-6)

### Step 3.1: Recompile Trigger
```c
// kernel/jit_optimizer.c

void jit_check_recompile(function_profiler_t* profiler, jit_engine_t* jit) {
    for (int i = 0; i < profiler->num_functions; i++) {
        func_profile_t* f = &profiler->functions[i];

        // Trigger recompile at thresholds
        if (f->calls == 100 && f->opt_level == 0) {
            jit_recompile(jit, f->bitcode, f->name, 1);  // O1
            f->opt_level = 1;
        }
        if (f->calls == 1000 && f->opt_level == 1) {
            jit_recompile(jit, f->bitcode, f->name, 2);  // O2
            f->opt_level = 2;
        }
    }
}
```

### Step 3.2: Atomic Code Swap
```c
typedef struct {
    void* code_ptr;
    atomic_uint version;
} func_entry_t;

// Recompile in background, then swap pointer
void jit_swap_code(func_entry_t* entry, void* new_code) {
    __atomic_store_n(&entry->code_ptr, new_code, __ATOMIC_RELEASE);
    __atomic_fetch_add(&entry->version, 1, __ATOMIC_ACQ_REL);
}
```

### Step 3.3: Module Execution with JIT
```c
int module_execute_jit(module_manager_t* mgr, const char* name) {
    func_entry_t* entry = find_function(mgr, name);

    // Load current code pointer atomically
    typedef int (*func_t)(void);
    func_t func = (func_t)__atomic_load_n(&entry->code_ptr, __ATOMIC_ACQUIRE);

    // Profile and execute
    uint64_t start = rdtsc();
    int result = func();
    uint64_t end = rdtsc();

    // Record profile
    function_profiler_record(name, end - start);

    // Check if recompile needed
    jit_check_recompile(&profiler, &jit_engine);

    return result;
}
```

## Phase 4: TinyLlama Integration (Week 7-10)

### Step 4.1: Layer-wise JIT
```c
// Each transformer layer is a JIT-compiled module
typedef struct {
    bitcode_module_t* attention;
    bitcode_module_t* feedforward;
    bitcode_module_t* layernorm;
} transformer_layer_t;

// Hot layers get O3, cold layers stay O0
```

### Step 4.2: Tensor Operation JIT
```c
// Specialize matmul for specific shapes
void* jit_matmul(int M, int N, int K, int opt_level);

// Cache compiled versions
matmul_cache[M][N][K] = jit_compile(...);
```

## Alternative: Custom Micro-JIT

If LLVM is too heavy, implement **custom x86 JIT**:

```c
// Generate x86 code directly for hot functions
typedef struct {
    uint8_t* code_buffer;
    size_t size;
} x86_code_t;

x86_code_t* jit_emit_loop_unroll(const loop_t* loop, int unroll_factor);
x86_code_t* jit_emit_matmul_simd(int M, int N, int K);
```

**Pros**:
- ~10KB code instead of 500KB
- No C++ runtime needed
- Full control

**Cons**:
- Limited optimizations
- Manual code generation
- No LLVM IR benefits

## Timeline

| Phase | Duration | Status |
|-------|----------|--------|
| Phase 1: Bitcode modules | 2 weeks | Not started |
| Phase 2: Minimal JIT | 2 weeks | Prototype exists (userspace) |
| Phase 3: Hot-path recompile | 2 weeks | Not started |
| Phase 4: TinyLlama integration | 4 weeks | Not started |

**Total**: ~10 weeks for full runtime JIT

## Next Steps (Immediate)

1. **Test userspace JIT interface** (kernel/jit_llvm18.cpp)
   ```bash
   make -f Makefile.jit test-interface
   ```

2. **Port minimal JIT to kernel**
   - Start with single bitcode module
   - JIT compile at O0
   - Execute and profile

3. **Implement recompile trigger**
   - Use function_profiler to detect hot functions
   - Recompile at O1/O2 thresholds

---

**Key Insight**: We already have most pieces! Just need to connect them:
- Module system ✅
- Profiling ✅
- JIT interface ✅ (userspace)
- Allocator ✅
- C++ runtime ✅

**Blocker**: LLVM in kernel needs ~500KB + careful runtime setup.

**Pragmatic approach**:
1. Keep AOT for now (current system works)
2. Add bitcode loading infrastructure
3. Gradually introduce JIT for hot functions only
4. Full JIT for TinyLlama (Phase 5)
