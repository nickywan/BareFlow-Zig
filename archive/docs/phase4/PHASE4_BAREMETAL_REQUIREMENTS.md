# Phase 4 - Bare-Metal LLVM Integration Requirements

**Date**: 2025-10-26 (Session 23)
**Goal**: Define requirements for integrating FULL LLVM 18 into bare-metal unikernel
**Status**: Session 23 complete - LLVM validated, requirements documented

---

## 🎯 Critical Philosophy: "Grow to Shrink"

### ⚠️ NEVER Optimize for Initial Size!

**The system MUST be large at start** - this is NOT a bug, it's a FEATURE!

```
"On s'en fiche de la taille initiale!"

Boot 1:     [60-118MB]  FULL LLVM + app IR  → VOULU! Profile TOUT
            ↓ Auto-profiling universel
Boot 100:   [30MB]      Hot paths JIT O0→O3
            ↓ Auto-optimization
Boot 500:   [10MB]      Dead code éliminé
            ↓ Convergence progressive
Boot 1000:  [2-5MB]     Pure native export
```

### Why Start Big?

1. **Universal Profiling**: Interpreter runs ALL code → profiles EVERYTHING
2. **Full Optimization**: All LLVM passes available (O0→O3)
3. **Dead Code Detection**: Track which 99.83% is never used
4. **Progressive Convergence**: Size reduction comes FROM usage, not guessing

**Result**: The system self-optimizes to minimal size through actual usage patterns, not manual constraints.

---

## ✅ Session 23 Validation Results

### LLVM 18 Installation Verified

**Status**: ✅ COMPLETE

| Component | Size | Status |
|-----------|------|--------|
| **libLLVM-18.so.1** | 118 MB | ✅ Installed |
| **Total installation** | 545 MB | ✅ Full |
| **Components** | 220 | ✅ All available |
| **Version** | 18.1.8 | ✅ Latest stable |

### Key Components Validated

- ✅ **Interpreter**: LLVM IR interpreter working
- ✅ **OrcJIT**: Just-in-time compiler functional
- ✅ **JITLink**: Modern JIT linker available
- ✅ **X86 Backend**: 32-bit and 64-bit targets
- ✅ **Optimization Passes**: O0, O1, O2, O3, Os, Oz
- ✅ **Tiered Compilation**: O0→O3 automatic progression

### Phase 3 Validation Suite Results

**Test**: `test_llvm_interpreter`
- AOT baseline: 0.471 ms
- Interpreter: 137.9 ms (292× slower)
- JIT: 0.360 ms (383× faster than interpreter)
- **Result**: ✅ 383× speedup validated

**Test**: `test_llvm_validation`
- LLVM size: 545 MB (FULL installation)
- Components: 220 available
- OrcJIT compilation: ✅ PASS (fib(10) = 55)
- All optimization passes: ✅ Verified

**Conclusion**: FULL LLVM 18 ready for bare-metal integration.

---

## 📋 Bare-Metal Integration Requirements

### 1. Boot Image Size

**Constraint**: QEMU virtual disk
**Target**: 60-118 MB bootable image (Boot 1)

**Components**:
```
Boot Image (60-118 MB):
├─ Bootloader (Stage 1+2):    8 KB
├─ Kernel runtime:            15 KB (kernel_lib.a)
├─ Application IR:            ~100 KB (TinyLlama inference in IR)
├─ LLVM runtime:              118 MB (libLLVM-18.so equivalent)
└─ FAT16 filesystem:          Headers + snapshot storage
```

**Strategy**: Create large bootable disk image (ISO or raw)
- Use multiboot or custom bootloader
- Load LLVM + app into memory
- Initialize LLVM JIT in bare-metal mode

---

### 2. Memory Requirements

**Target**: 256 MB RAM (typical for bare-metal development)

**Memory Layout**:
```
0x00000000 - 0x000FFFFF:  Low memory (BIOS, IVT)
0x00100000 - 0x0FFFFFFF:  Kernel + LLVM (~118 MB)
0x10000000 - 0x1FFFFFFF:  Heap (malloc for LLVM allocations)
0x20000000 - 0x2FFFFFFF:  Code cache (JIT-compiled code)
0x30000000 - 0x3FFFFFFF:  IR storage + snapshot buffer
```

**Key Constraints**:
- LLVM needs ~150-200 MB for full functionality
- JIT code cache: ~50 MB (hot code)
- Heap: ~50 MB (allocations, temporary buffers)
- Total: ~256 MB acceptable

---

### 3. LLVM Bare-Metal Port Requirements

#### 3.1 No Standard Library Dependencies

**Problem**: LLVM uses C++ stdlib (libstdc++, libc++)

**Solution**: Custom bare-metal C++ runtime
```c++
// kernel_lib/cpp_runtime/
├─ new.cpp              // operator new/delete
├─ exception.cpp        // no-op exception handlers (-fno-exceptions)
├─ rtti.cpp             // no-op RTTI stubs (-fno-rtti)
├─ iostream.cpp         // minimal std::cout → serial port
├─ string.cpp           // std::string → custom implementation
└─ vector.cpp           // std::vector → custom allocator
```

**Flags**:
- `-fno-exceptions` (no C++ exception handling)
- `-fno-rtti` (no runtime type information)
- `-nostdlib` (no standard library)
- `-fno-use-cxa-atexit` (no atexit support)

---

#### 3.2 Custom Memory Allocator

**Current**: kernel_lib/memory/malloc.c (simple bump allocator)

**LLVM Needs**:
- `malloc()`, `free()`, `realloc()`, `calloc()`
- Thread-safe allocations (single-threaded in bare-metal OK)
- Large allocations (~100 MB for LLVM internals)

**Enhancement Required**:
```c
// kernel_lib/memory/llvm_allocator.c

// Current: Simple bump allocator (15 KB)
// Required: Free-list allocator with:
//   - Large block support (up to 10 MB)
//   - Reasonable free() support (for LLVM cleanup)
//   - Memory alignment (16-byte for SIMD)
```

**Implementation Plan**:
1. Keep simple allocator for kernel
2. Add LLVM-specific allocator (dlmalloc-style)
3. Override LLVM allocator hooks

---

#### 3.3 System Call Stubs

**LLVM Dependencies**:
```c
// Required stubs (no-op or minimal implementation):
void __cxa_atexit(void (*)(void*), void*, void*) {}
void __cxa_finalize(void*) {}
int __cxa_thread_atexit_impl(void (*)(void*), void*, void*) { return 0; }

// File I/O (for error reporting only):
int fprintf(FILE* stream, const char* format, ...) {
    // Redirect to serial port
    serial_printf(format, ...);
    return 0;
}
```

---

#### 3.4 Threading & Synchronization

**Current State**: Single-threaded bare-metal

**LLVM Assumptions**:
- May use mutexes internally
- May check thread IDs

**Solution**: Stub out threading primitives
```c++
// kernel_lib/cpp_runtime/threading.cpp

// No-op mutex (single-threaded)
struct pthread_mutex_t { int dummy; };
int pthread_mutex_lock(pthread_mutex_t*) { return 0; }
int pthread_mutex_unlock(pthread_mutex_t*) { return 0; }
int pthread_mutex_init(pthread_mutex_t*, void*) { return 0; }
```

**Future**: Add actual bare-metal mutexes if needed

---

### 4. Build System Integration

#### 4.1 Linking Strategy

**Phase 3 Finding**: Dynamic linking works best for LLVM 18

**Bare-Metal Problem**: No dynamic linker available

**Solution**: Static linking with custom LLVM build

```bash
# Option 1: Use pre-built LLVM with custom stubs
clang++ -m32 -nostdlib -static \
    main.cpp \
    kernel_lib.a \
    cpp_runtime.a \
    -lLLVM-18 \
    -o kernel.elf

# Option 2: Build minimal LLVM from source
# (See Phase 3.5 - 99.83% dead code elimination)
cmake -DLLVM_TARGETS_TO_BUILD=X86 \
      -DLLVM_ENABLE_PROJECTS="clang" \
      -DLLVM_BUILD_LLVM_DYLIB=OFF \
      -DLLVM_LINK_LLVM_DYLIB=OFF \
      -DCMAKE_BUILD_TYPE=MinSizeRel \
      -DLLVM_ENABLE_THREADS=OFF \
      -DLLVM_ENABLE_EXCEPTIONS=OFF \
      -DLLVM_ENABLE_RTTI=OFF \
      ...
```

**Target**: 2-5 MB minimal LLVM (X86 + OrcJIT only)

---

#### 4.2 Boot Integration

**Current Bootloader**: 2-stage (512B + 4KB)

**New Bootloader Requirements**:
```
Stage 1 (512 bytes):
  - Load Stage 2 from disk

Stage 2 (4 KB):
  - Enable A20 line
  - Setup GDT (32-bit protected mode)
  - Load kernel + LLVM (up to 118 MB)
  - Jump to kernel entry

Kernel Entry:
  - Initialize heap (256 MB)
  - Initialize LLVM runtime
  - Load application IR
  - Start interpreter mode
```

**Disk Layout**:
```
Sector 0:       Stage 1 bootloader (512 bytes)
Sector 1-8:     Stage 2 bootloader (4 KB)
Sector 9-N:     Kernel + LLVM binary (118 MB)
Sector N+1...:  FAT16 filesystem (snapshots)
```

**Challenges**:
- Loading 118 MB from disk (~20 seconds @ 6 MB/s)
- May need compression (gzip → ~40 MB)
- Consider ramdisk for speed

---

### 5. JIT Runtime Integration

#### 5.1 LLVM Initialization

```c++
// kernel_lib/jit/llvm_init.cpp

void llvm_baremetal_init() {
    // 1. Initialize LLVM subsystems
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // 2. Create execution session
    auto EPC = SelfExecutorProcessControl::Create();
    auto ES = std::make_unique<ExecutionSession>(std::move(EPC));

    // 3. Setup JIT builder
    auto JTMB = JITTargetMachineBuilder::detectHost();
    auto DL = JTMB->getDefaultDataLayoutForTarget();

    // 4. Create LLJIT instance
    auto J = LLJITBuilder()
        .setExecutorProcessControl(std::move(EPC))
        .setDataLayout(*DL)
        .create();

    // 5. Add IR module
    auto Ctx = std::make_unique<LLVMContext>();
    auto M = parseIRFile("app.ll", Err, *Ctx);
    J->addIRModule(ThreadSafeModule(std::move(M), std::move(Ctx)));
}
```

---

#### 5.2 Code Cache Management

**Problem**: JIT-compiled code needs executable memory

**Solution**: Use page tables to mark memory executable

```c
// kernel_lib/memory/code_cache.c

#define CODE_CACHE_BASE 0x20000000
#define CODE_CACHE_SIZE (50 * 1024 * 1024)  // 50 MB

void* allocate_executable_memory(size_t size) {
    static uintptr_t next_addr = CODE_CACHE_BASE;

    void* addr = (void*)next_addr;
    next_addr += ALIGN(size, 4096);  // Page-aligned

    // Mark pages as executable in page tables
    mark_pages_executable(addr, size);

    return addr;
}
```

**Integration with LLVM**:
- Override `allocateCodeSection()` in SectionMemoryManager
- Use custom allocator for code cache
- Handle relocations manually

---

#### 5.3 Profiling Integration

**Goal**: Track ALL function calls for optimization

**Implementation**:
```c
// kernel_lib/jit/profiler.c

struct FunctionProfile {
    const char* name;
    uint64_t call_count;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
};

// Injected at function entry
void __profile_enter(const char* name) {
    uint64_t start = rdtsc();
    // Store start time in TLS or stack
}

void __profile_exit(const char* name) {
    uint64_t end = rdtsc();
    // Update FunctionProfile
}
```

**LLVM Integration**:
- Use LLVM instrumentation pass
- Inject calls to profiler at IR level
- Collect data during interpreter execution

---

### 6. Persistence (Snapshot System)

#### 6.1 FAT16 Integration

**Current**: kernel_lib has basic FAT16 read support

**Required**: Add write support for snapshots

```c
// kernel_lib/fs/fat16_write.c

int fat16_write_file(const char* path, void* data, size_t size) {
    // 1. Find free clusters
    // 2. Allocate chain
    // 3. Write data sectors
    // 4. Update directory entry
    // 5. Update FAT
}
```

---

#### 6.2 Snapshot Format

**Based on Phase 3.6 findings**:

```c
// kernel_lib/jit/snapshot.h

#define SNAPSHOT_MAGIC 0x534E4150  // "SNAP"

struct SnapshotHeader {
    uint32_t magic;              // 0x534E4150
    uint32_t version;            // 1
    uint32_t num_functions;      // Number of hot functions
    uint32_t boot_count;         // Which boot created this
    uint64_t total_size;         // Size of all code
};

struct FunctionEntry {
    char name[64];               // Function name
    uint32_t code_size;          // Size of machine code
    uint64_t call_count;         // Number of calls
    uint64_t avg_cycles;         // Average execution time
    uint8_t optimization_level;  // 0=O0, 1=O1, 2=O2, 3=O3
    uint8_t code[];              // Machine code follows
};
```

**Snapshot File**:
```
/snapshots/boot_0001.snap  → Boot 1 (no snapshot, all IR)
/snapshots/boot_0010.snap  → Boot 10 (O0 compiled hot paths)
/snapshots/boot_0100.snap  → Boot 100 (O3 compiled, ~30 MB)
/snapshots/boot_1000.snap  → Boot 1000 (pure native, ~5 MB)
```

---

#### 6.3 Snapshot Loading

**Boot Sequence**:
```c
void boot_with_snapshots() {
    // 1. Load latest snapshot
    Snapshot* snap = load_latest_snapshot();

    if (snap) {
        // 2. Load pre-compiled functions
        for (int i = 0; i < snap->num_functions; i++) {
            FunctionEntry* fn = &snap->functions[i];
            void* code = allocate_executable_memory(fn->code_size);
            memcpy(code, fn->code, fn->code_size);
            register_function(fn->name, code);
        }

        // 3. Continue profiling cold code
        llvm_interpreter_mode();
    } else {
        // 4. No snapshot - start from IR
        llvm_interpreter_mode();
    }
}
```

---

### 7. Convergence Detection

**Goal**: Automatically transition Boot 1 → Boot 1000

**Strategy**:
```c
// kernel_lib/jit/convergence.c

struct ConvergenceState {
    uint32_t boot_count;
    uint32_t total_functions;
    uint32_t hot_functions;     // Called > 100 times
    uint32_t jit_functions;     // JIT-compiled
    uint64_t total_code_size;
    double   optimization_ratio; // jit_functions / hot_functions
};

void check_convergence(ConvergenceState* state) {
    // Thresholds from Phase 3.4:
    // - O0 at 100 calls
    // - O1 at 1,000 calls
    // - O2 at 10,000 calls
    // - O3 at 100,000 calls

    if (state->optimization_ratio > 0.95) {
        // 95% of hot functions are optimized
        export_native_snapshot();
        print("Convergence achieved! Export to native.");
    }
}
```

**Milestones**:
- Boot 10: ~10% hot functions identified
- Boot 100: ~50% functions JIT-compiled
- Boot 500: ~90% optimization complete
- Boot 1000: 95%+ converged → pure native

---

## 📊 Size Evolution Projections

### Based on Phase 3 Results

| Boot Count | Size | Description | Strategy |
|------------|------|-------------|----------|
| **Boot 1** | 118 MB | FULL LLVM + app IR | Interpreter ALL code |
| **Boot 10** | 100 MB | LLVM + O0 hot paths | Profile 100% → JIT 10% |
| **Boot 100** | 60 MB | LLVM + O3 hot paths | JIT 50%, remove cold IR |
| **Boot 500** | 30 MB | Minimal LLVM + natives | 90% native, LLVM for edge cases |
| **Boot 1000** | 5 MB | Pure native binary | LLVM removed, snapshot only |

**Phase 3.6 Validation**: 118 MB → 20 KB (6000× reduction) proven achievable

---

## 🎯 Implementation Phases

### Phase 4.1: Custom LLVM Build (Sessions 23-24) ✅
**Status**: Session 23 COMPLETE - LLVM validated

**Next**:
- [x] Verify FULL LLVM 18 installation
- [x] Confirm all optimization passes
- [x] Test Phase 3 validation suite
- [x] Document bare-metal requirements

### Phase 4.2: Bare-Metal Port (Sessions 25-26)
**Goal**: Port LLVM to bare-metal environment

**Tasks**:
- [ ] Implement C++ runtime (new, delete, no-exceptions)
- [ ] Create LLVM-compatible allocator
- [ ] Stub out system dependencies
- [ ] Build minimal LLVM statically
- [ ] Test with simple IR function

### Phase 4.3: Boot Integration (Sessions 27-28)
**Goal**: Boot unikernel with LLVM

**Tasks**:
- [ ] Create 118 MB bootable image
- [ ] Load LLVM into memory
- [ ] Initialize JIT runtime
- [ ] Run interpreter on app IR
- [ ] Verify execution

### Phase 4.4: Persistence (Sessions 29-30)
**Goal**: Implement snapshot system

**Tasks**:
- [ ] Add FAT16 write support
- [ ] Implement snapshot save/load
- [ ] Test convergence (Boot 1 → 1000)
- [ ] Measure size reduction
- [ ] Validate "Grow to Shrink"

---

## 🚀 Success Criteria

### Session 23 (COMPLETE) ✅
- [x] FULL LLVM 18 installed (545 MB)
- [x] All optimization passes verified (O0-O3)
- [x] Phase 3 tests pass (383× speedup)
- [x] Requirements documented

### Phase 4 Overall
- [ ] Boot with FULL LLVM (118 MB) working
- [ ] Interpreter executes ALL code
- [ ] JIT compilation successful
- [ ] Profiling tracks all functions
- [ ] Convergence to 5 MB after 1000 boots

---

## 📂 Files Created/Modified

### Documentation
- `docs/phase4/PHASE4_BAREMETAL_REQUIREMENTS.md` - This document

### Code (To Be Created)
- `kernel_lib/cpp_runtime/` - C++ bare-metal runtime
- `kernel_lib/jit/llvm_init.cpp` - LLVM initialization
- `kernel_lib/jit/profiler.c` - Universal profiling
- `kernel_lib/jit/snapshot.c` - Snapshot save/load
- `kernel_lib/memory/llvm_allocator.c` - LLVM-compatible malloc
- `kernel_lib/fs/fat16_write.c` - FAT16 write support

### Build System
- `llvm_build/` - Custom LLVM build scripts
- `Makefile.llvm` - LLVM integration

---

## 🔗 References

### Phase 3 Results
- **PHASE3_3_RESULTS.md**: 399× interpreter→JIT speedup
- **PHASE3_4_TIERED_JIT.md**: Tiered O0→O3 compilation
- **PHASE3_5_DCE_RESULTS.md**: 99.83% dead code analysis
- **PHASE3_6_NATIVE_EXPORT.md**: 6000× size reduction proof

### Architecture
- **README.md**: "Grow to Shrink" philosophy
- **ARCHITECTURE_UNIKERNEL.md**: Unikernel design (28 KB)
- **ROADMAP.md**: Project timeline and phases

### Implementation
- **kernel_lib/**: Runtime library (15 KB)
- **tests/phase3/**: Validation programs
- **tinyllama/**: Unikernel application

---

**Status**: Session 23 complete - Ready to start bare-metal port (Session 25)
**Next**: Implement C++ runtime and LLVM allocator
**Goal**: Boot unikernel with FULL LLVM by Session 28

---

## ⚠️ Critical Reminders

1. **Never minimize LLVM upfront** - Use FULL 118 MB
2. **Profile everything first** - Interpreter mode is intentional
3. **Trust the convergence** - Size reduction comes from usage
4. **Persistence is key** - Snapshots make convergence permanent
5. **Validate continuously** - Compare against Phase 3 metrics

**"On s'en fiche de la taille initiale - on optimise par convergence!"**
