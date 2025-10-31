# Session 36 - Architecture Refactoring & llvm-libc Strategy

**Date**: 2025-10-26
**Status**: Partially Complete - Bugs Discovered
**Branch**: feat/true-jit-unikernel

---

## 🎯 Session Goals

1. **Document llvm-libc strategy** - Clarify hybrid runtime approach
2. **Resolve return crash bug** - Refactor code architecture to avoid bare-metal return issues
3. **Test TinyLlama model loading** - Validate new architecture

---

## ✅ Completed Tasks

### 1. llvm-libc Strategy Documentation

**Created**: `docs/architecture/LLVM_LIBC_STRATEGY.md` (310 lines)

**Key Decision**: **HYBRID Approach**
- **Custom kernel_lib** for I/O & CPU (VGA, serial, keyboard, PIC, IDT, rdtsc)
  - Hardware-specific, non-JIT-optimizable
  - Direct MMIO/port access
  - Already functional and compact (3 KB)

- **llvm-libc** for Memory & String functions (malloc, memcpy, memset, strlen, etc.)
  - Written in pure C → JIT-optimizable
  - Auto-vectorization potential (e.g., memcpy AVX2 → 10× speedup)
  - Enables universal profiling (app + LLVM + libc)

**Rationale**:
```c
// Boot 1: Generic memcpy (1 byte/cycle = 512 cycles for 512 bytes)
void* memcpy(void* dst, const void* src, size_t n);

// Boot 100: JIT observes size=512, alignment=16
void* __llvm_memcpy_512_aligned16(void* dst, const void* src) {
    // AVX2: 32 bytes/instruction → ~50 cycles for 512 bytes
    // = 10× FASTER!
}
```

**Implementation Timeline**: Phase 6 (Sessions 41-45)

**Updated Documentation**:
- `CLAUDE.md` - Added "Runtime Strategy: Hybrid kernel_lib + llvm-libc" section
- `README.md` - Updated Phase 3 architecture diagram to show hybrid approach

---

### 2. Code Architecture Refactoring

**Problem** (Session 35): Monolithic `tinyllama_create_model()` crashed on `return` statement
- 200+ lines, 20+ malloc calls, complex control flow
- Even simple `return 0` caused triple-fault

**Solution**: Split into 5 simple helper functions + orchestrator

**New Architecture**:

```c
// Step 1: Allocate structure (15 lines, returns int)
static int tinyllama_alloc_structure(TinyLlamaModel** out_model);

// Step 2: Set configuration (20 lines, returns int)
static int tinyllama_set_config(TinyLlamaModel* model);

// Step 3: Allocate layers array (18 lines, returns int)
static int tinyllama_alloc_layers_array(TinyLlamaModel* model);

// Step 4: Allocate single layer (42 lines, returns int)
static int tinyllama_alloc_single_layer(TransformerLayer* layer, uint32_t hidden_size);

// Step 5: Allocate final layer norm (24 lines, returns int)
static int tinyllama_alloc_final_norm(TinyLlamaModel* model);

// Main orchestrator - simple and clean (40 lines)
int tinyllama_create_model(TinyLlamaModel** out_model) {
    if (tinyllama_alloc_structure(out_model) != 0) return -1;
    TinyLlamaModel* model = *out_model;
    if (tinyllama_set_config(model) != 0) goto error;
    if (tinyllama_alloc_layers_array(model) != 0) goto error;
    if (tinyllama_alloc_single_layer(&model->layers[0], ...) != 0) goto error;
    if (tinyllama_alloc_final_norm(model) != 0) goto error;
    return 0;
error:
    free(model->layers); free(model); *out_model = NULL; return -1;
}
```

**Code Reduction**: 552 lines → 380 lines (31% reduction)

**Files Modified**:
- `tests/phase4/qemu_llvm_64/tinyllama_model.c` - Complete refactor
- `tests/phase4/qemu_llvm_64/tinyllama_model.h` - Cleaned prototypes
- `tests/phase4/qemu_llvm_64/kernel.cpp` - Simplified caller
- Backup: `tinyllama_model.c.session35.bak`

---

## 🐛 Bugs Discovered

### Bug 1: `serial_put_uint()` Crash

**Symptom**: Calling `serial_put_uint(size)` produces MASSIVE output (1000+ spaces?) then crashes

**Example**:
```c
serial_put_uint(5632);  // Expect: "5632"
// Actual: [massive output of spaces/garbage] → CRASH
```

**Workaround**: Avoid `serial_put_uint()` entirely - use fixed strings

**Location**: `kernel_lib/io/serial.c:serial_put_uint()`

**Priority**: Medium (workaround available)

---

### Bug 2: `malloc()` Crash on Small Allocation

**Symptom**: `malloc(~5KB)` crashes BEFORE returning NULL

**Expected Behavior**:
```c
void* ptr = malloc(5632);
if (!ptr) {
    serial_puts("FAILED\n");  // Should reach here if out of memory
}
```

**Actual Behavior**:
```
[TinyLlama] Allocating layers array (~5KB)...
[TinyLlama] Allocating layer components...  ← Skips to next function!
```

Neither "OK" nor "FAILED" is printed → **crash happens INSIDE malloc()**

**Test Output**:
```
=== TinyLlama Model Creation (Session 36) ===
[TinyLlama] Allocating structure... OK
[TinyLlama] Config...12345678 OK
[TinyLlama] Allocating layers array (~5KB)... [TinyLlama] Allocating layer components...
```

**Hypothesis**:
- malloc() runs out of memory but crashes instead of returning NULL
- Possible corrupt heap state from Session 35 changes (64 MB heap)
- May be alignment issue or heap pointer corruption

**Location**: `kernel_lib/memory/malloc_bump.c`

**Priority**: **HIGH** - Blocks all model loading

---

## ✅ Partial Validation Results

### What Works ✅

1. **Structure allocation** - `malloc(sizeof(TinyLlamaModel))` succeeds
2. **Configuration** - All 8 struct field assignments succeed (12345678 printed)
3. **Return from small functions** - No return crash with new architecture!

### What Fails ❌

1. **Layers array allocation** - `malloc(~5KB)` crashes
2. **Layer component allocation** - Never reached due to #1

---

## 📊 Test Results

**Kernel Size**: 18,528 bytes (18 KB)

**Boot Sequence**: ✅ Successful
- Multiboot2 boot
- Serial I/O working
- Paging initialized (2 MB pages, 0-512 MB identity mapped)
- 64 MB heap initialized
- kernel_lib_llvm.a linked (28 KB)

**Model Creation Progress**:
```
Step 1: ✅ Allocate structure (OK)
Step 2: ✅ Set config (OK)
Step 3: ❌ Allocate layers array (CRASH in malloc)
Step 4: ⏭️  Not reached
Step 5: ⏭️  Not reached
```

---

## 🔍 Key Discoveries

### Discovery 1: Refactoring Strategy Works!

**Before** (Session 35):
- Monolithic function with 200+ lines
- Crashed on `return 0;`
- Even `void` return crashed on final assignment

**After** (Session 36):
- 5 simple functions (<45 lines each)
- All functions return cleanly (tested: structure, config)
- No return crash observed!

**Lesson**: Bare-metal compiler optimizations struggle with complex functions. Keep functions small and simple.

---

### Discovery 2: `serial_put_uint()` is Broken

**Investigation**:
```c
unsigned long size = 5632;
serial_put_uint(size);  // Expected: "5632"
// Actual: [MASSIVE output] → system crash
```

Output shows 1000+ spaces/garbage characters before crash.

**Possible Causes**:
- Buffer overflow in conversion routine
- Stack corruption during digit extraction
- Incorrect loop termination

**Impact**: Cannot debug numeric values in bare-metal

---

### Discovery 3: malloc() Does Not Return NULL on Failure

**Standard behavior** (expected):
```c
void* ptr = malloc(size);
if (ptr == NULL) {
    // Handle allocation failure
}
```

**Bare-metal behavior** (actual):
```c
void* ptr = malloc(size);  // ← Crashes HERE if out of memory
// Never returns if allocation fails!
```

**Evidence**:
- Serial output stops immediately after malloc() call
- No "OK" or "FAILED" message printed
- System triple-faults (reboot)

**Implication**: Need to fix malloc_bump.c to properly return NULL instead of crashing

---

## 📝 User Feedback - TinyLlama Compilation Note

**User reminder**:
> "N'oublie pas que tinyllama est un code disponible sur le net donc sans doute compilable par nos soins"

TinyLlama is **open source** and could be compiled directly rather than reimplemented from scratch.

**Future consideration**: Download official TinyLlama C implementation and compile for bare-metal, rather than writing our own inference code.

---

## 🔧 Next Steps (Session 37)

### Priority 1: Fix malloc() Bug 🔴

**Task**: Investigate why `malloc(5632)` crashes

**Files to check**:
- `kernel_lib/memory/malloc_bump.c`
- `kernel_lib/memory/paging.c` (heap initialization)

**Debug approach**:
1. Add debug prints inside malloc_bump.c
2. Check heap boundaries and current pointer
3. Verify 64 MB heap is correctly initialized
4. Test with smaller allocations (1 KB, 2 KB, 4 KB progression)

**Expected fix**: Make malloc() return NULL when out of memory instead of crashing

---

### Priority 2: Fix serial_put_uint() Bug 🟡

**Task**: Debug or rewrite `serial_put_uint()`

**Files**: `kernel_lib/io/serial.c`

**Workaround**: Use fixed strings for now

---

### Priority 3: Complete Model Loading Tests 🟢

**After malloc fix**:
1. Test layers array allocation
2. Test single layer allocation (4 MB per layer for Q/K/V/O matrices)
3. Test weight loading
4. Measure actual memory usage

---

## 📚 Architecture Documentation

**New Files Created**:
- `docs/architecture/LLVM_LIBC_STRATEGY.md` - Comprehensive strategy document

**Updated Files**:
- `CLAUDE.md` - Runtime strategy section
- `README.md` - Architecture diagram

**Backup Files**:
- `tinyllama_model.c.session35.bak` - Original monolithic version

---

## 💡 Lessons Learned

### Bare-Metal C Gotchas

1. **Complex functions crash on return**
   - Solution: Keep functions <50 lines, simple control flow
   - Each function should have ONE clear purpose

2. **Library functions may not behave as expected**
   - `serial_put_uint()` broken (output garbage)
   - `malloc()` crashes instead of returning NULL
   - Always test library functions thoroughly

3. **Debug incrementally**
   - Add serial_puts() after EVERY statement when debugging crashes
   - Use single-character debug outputs ("1", "2", "3"...) to minimize crash risk

4. **Avoid complex expressions in bare-metal**
   - `serial_put_uint(LLAMA_N_LAYERS * sizeof(TransformerLayer))` → CRASH
   - Break into simple statements with intermediate variables

---

## 📊 Statistics

| Metric | Value |
|--------|-------|
| **Code Reduction** | 552 → 380 lines (31%) |
| **Functions Created** | 5 helper + 1 orchestrator |
| **Max Function Size** | 42 lines (tinyllama_alloc_single_layer) |
| **Documentation Added** | 310 lines (LLVM_LIBC_STRATEGY.md) |
| **Bugs Found** | 2 (serial_put_uint, malloc crash) |
| **Kernel Size** | 18 KB |
| **Successful Steps** | 2/5 (structure, config) |

---

## 🎯 Success Criteria

### Session 36 Goals

| Goal | Status | Notes |
|------|--------|-------|
| Document llvm-libc strategy | ✅ **COMPLETE** | 310-line strategy doc created |
| Update CLAUDE.md & README.md | ✅ **COMPLETE** | Hybrid runtime documented |
| Refactor code architecture | ✅ **COMPLETE** | 5 simple functions + orchestrator |
| Test model creation | ⚠️ **PARTIAL** | 2/5 steps pass, malloc bug blocks rest |

### Blockers for Session 37

1. 🔴 **malloc() crash** - HIGH priority, blocks all progress
2. 🟡 **serial_put_uint() bug** - Medium priority, workaround exists

---

**Status**: Session 36 achieved major progress on architecture and documentation, but revealed critical malloc bug that must be fixed before continuing with model loading.

**Next Session Focus**: Debug malloc_bump.c and fix allocation crash.
