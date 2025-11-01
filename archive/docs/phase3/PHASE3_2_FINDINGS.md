# Phase 3.2 - Full Static Link Findings

**Date**: 2025-10-26
**Goal**: Create 60MB bootable binary with full LLVM statically linked
**Status**: ⚠️ BLOCKED - System LLVM doesn't support full static linking

---

## 🧪 Tests Performed

### Test 1: Minimal Static Link (30 LLVM libs)
```bash
make -f Makefile.full_static test_minimal_static
```

**Result**: ✗ FAILED
- Many undefined references (missing LLVM libs)
- File created but invalid (type "data", not executable)
- Size: 27MB

**Missing symbols**:
- `llvm::jitlink::Section::~Section()`
- `llvm::SubtargetFeatures::SubtargetFeatures()`
- `llvm::SelectionDAGTargetInfo::~SelectionDAGTargetInfo()`
- `llvm::initializeGlobalISel()`
- Many more...

### Test 2: Full Static Link (ALL LLVM libs with --whole-archive)
```bash
make -f Makefile.full_static test_full_static
```

**Result**: ✗ FAILED
- Still many undefined references (missing system libs)
- File created but invalid (type "data", not executable)
- Size: 110MB (!!)

**Missing system libraries**:
- `libpolly.so` - Polly optimization plugin
- `libffi` - Foreign function interface (ffi_*)
- `libedit` - Line editor (el_*, history_*)
- `libzstd` - Compression (ZSTD_*)
- `libpfm` - Performance monitoring (pfm_*)

---

## 🔍 Root Cause Analysis

### Problem: System LLVM Not Built for Static Linking

Ubuntu's LLVM 18 package is built with:
- **Shared libraries** as primary target (.so files)
- **Static archives** (.a) available but incomplete
- **Dependencies on system shared libs** (Polly, libedit, libffi, etc.)

**Why static linking fails**:
1. LLVM static archives reference external shared libs
2. These shared libs don't have static equivalents installed
3. Would need ALL of: libffi-dev, libedit-dev, libzstd-dev, libpfm4-dev, libpolly-18-dev (doesn't exist!)
4. Even with all deps, Polly plugin is ONLY available as .so

---

## 📊 Measurements Achieved

| Metric | Value | Notes |
|--------|-------|-------|
| **Dynamic binary** (Phase 3.1) | 31KB | + 118MB libLLVM-18.so |
| **Minimal static attempt** | 27MB | Invalid, undefined refs |
| **Full static attempt** | 110MB | Invalid, undefined refs |
| **System LLVM .so** | 118MB | Unstripped |
| **LLVM static archives (total)** | ~60MB | Uncompressed, all .a files |

---

## ✅ What We Learned

### 1. LLVM Component Sizes (individual .a files)
```
libLLVMCodeGen.a:       18MB (largest)
libLLVMX86CodeGen.a:    11MB
libLLVMAnalysis.a:      11MB
libLLVMCore.a:          8.5MB
libLLVMOrcJIT.a:        5.1MB
libLLVMX86Desc.a:       5.2MB
... (30+ more libs)
```

### 2. Dynamic Linking Works Perfectly
- test_jit_minimal (Phase 3.1): ✅ SUCCESS
- 31KB binary + 118MB shared lib
- JIT compilation verified working
- Simple to build, no dependency hell

### 3. Static Linking Requires Custom LLVM Build
- Can't use system LLVM for full static
- Would need: `-DLLVM_LINK_LLVM_DYLIB=OFF`, `-DLLVM_BUILD_LLVM_C_DYLIB=OFF`
- Estimated build time: **2-3 hours**
- Result size: probably still 60-100MB before stripping

---

## 🎯 Proposed Solutions for Phase 3.2

### Option A: Build Custom LLVM (Long-Term, Correct)

**Steps**:
```bash
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build && cd build

cmake -G Ninja ../llvm \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_ENABLE_RTTI=OFF \
  -DLLVM_ENABLE_EH=OFF \
  -DLLVM_BUILD_TOOLS=OFF \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF \
  -DLLVM_LINK_LLVM_DYLIB=OFF \
  -DLLVM_BUILD_LLVM_C_DYLIB=OFF \
  -DBUILD_SHARED_LIBS=OFF

ninja  # 2-3 hours
```

**Pros**:
- ✅ True 60MB static binary achievable
- ✅ Full control over LLVM build
- ✅ Can optimize for size (MinSizeRel, LTO)
- ✅ Aligns with "Grow to Shrink" philosophy

**Cons**:
- ⏳ Takes 2-3 hours to build
- 💾 Requires ~10GB disk space during build
- 🔧 More complex setup

### Option B: Hybrid Approach (Pragmatic, Fast)

**Strategy**: Start with **dynamic linking**, transition to static later

**Phase 3.2 revised**:
1. ✅ Use dynamic LLVM (test_jit_minimal works!)
2. ✅ Implement LLVM Interpreter mode in userspace
3. ✅ Test tiered JIT (O0 → O3) with dynamic libs
4. ✅ Validate all functionality
5. **Later**: Build custom LLVM when ready for bare-metal

**Pros**:
- ✅ Can start Phase 3.3 (Interpreter) immediately
- ✅ No waiting for LLVM build
- ✅ Easier debugging (dynamic linking)
- ✅ Can switch to static later

**Cons**:
- ⚠️ Can't measure true bare-metal size yet
- ⚠️ Dynamic libs not viable for final bare-metal

### Option C: Minimal Embedded LLVM (Research Needed)

**Idea**: Use LLVM in "minimal embedded mode"
- Only OrcJIT + X86 backend
- No Polly, no LineEditor, no Exegesis
- Hand-pick exactly what's needed

**Would require**:
- Deep dive into LLVM build system
- Manually exclude unwanted components
- Potentially patch LLVM sources

**Status**: Not explored yet

---

## 📝 Recommendation

**For next session**: Choose **Option B (Hybrid)**

**Rationale**:
1. You want to pause after Phase 3.2
2. Dynamic linking WORKS and lets us continue
3. Can implement Interpreter + Tiered JIT in userspace
4. Build custom LLVM later when transitioning to bare-metal
5. Aligns with "Grow to Shrink" - start working, optimize later

**Next concrete steps**:
1. Create `test_llvm_interpreter.cpp` (use dynamic LLVM)
2. Implement basic IR interpretation
3. Add tiered JIT (O0 → O1 → O2 → O3)
4. Measure performance gains
5. **Then pause and decide**: custom LLVM build or continue with dynamic?

---

## 🔄 Alternative: Pivot to LLVM-libc First

**Another option**: Instead of fighting static LLVM linking now, focus on **LLVM-libc** integration:

**Why**:
- LLVM-libc is smaller, easier to static link
- Can replace `kernel_lib/stdlib.c` now
- Still progress toward bare-metal
- Less complex than full LLVM JIT

**Steps**:
1. Build LLVM-libc to static lib
2. Replace kernel_lib stdlib functions
3. Test in tinyllama unikernel
4. Measure benefits (cross-module optimization)

**Then** come back to JIT runtime later.

---

## 🎯 Decision Point

**Choose one**:
- **A**: Build custom LLVM (2-3h), get true static link
- **B**: Continue with dynamic LLVM, static later
- **C**: Pivot to LLVM-libc first, JIT runtime later

**Your call!**

What makes most sense given:
- Time available
- Research vs production goal
- Desire to see working JIT quickly

---

**Created**: 2025-10-26
**Status**: Awaiting decision on next steps
