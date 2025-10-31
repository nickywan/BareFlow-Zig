# Phase 4.1: LLVM Build Options Analysis

**Date**: 2025-10-26
**Session**: 23
**Objectif**: Choisir et configurer un JIT backend ≤5MB pour bare-metal

---

## 🎯 Requirement

**Target**: JIT compiler ≤5MB pour bare-metal unikernel

**Critères**:
1. **Size**: ≤5MB (vs 118MB current libLLVM-18.so)
2. **Performance**: <2× slower than AOT (validated 1.17× in Phase 3.4)
3. **Bare-metal**: No libc, no threads, no exceptions
4. **C integration**: Easy FFI with kernel_lib

---

## 🔬 Option 1: Custom LLVM Build

### Configuration Minimale

**CMake Options** (from research):
```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_BUILD_TOOLS=OFF \
  -DLLVM_INCLUDE_TOOLS=OFF \
  -DLLVM_BUILD_LLVM_DYLIB=OFF \
  -DLLVM_ENABLE_PROJECTS="" \
  -DLLVM_ENABLE_TERMINFO=OFF \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_ZSTD=OFF \
  -DLLVM_ENABLE_BACKTRACES=OFF \
  -DLLVM_ENABLE_CRASH_OVERRIDES=OFF \
  -DLLVM_ENABLE_LIBEDIT=OFF \
  -DLLVM_ENABLE_LIBPFM=OFF \
  -DLLVM_ENABLE_THREADS=OFF \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  ../llvm
```

### Expected Size

**Benchmarks from research**:
- Minimal LLVM build: ~34MB (source: boxbase.org 2018)
- With aggressive stripping: potentially 10-20MB
- **Our target**: 2-5MB

**Reality check**: Difficile d'atteindre <5MB avec LLVM complet

### Pros & Cons

✅ **Pros**:
- Already validated in Phase 3 (tests working)
- Full feature set (OrcJIT, optimization passes)
- Well documented
- Known performance (1.17× vs AOT)

❌ **Cons**:
- Likely >5MB even minimal
- Complex build process
- Heavy C++ runtime dependencies
- Large codebase (20M LOC)

### Estimated Effort

- Build configuration: 1-2 days
- Testing: 1 day
- Bare-metal port: 5-7 days
- **Total**: ~9-10 days

---

## 🔬 Option 2: QBE Compiler Backend

### Overview

**QBE** = Quick Backend
- **Language**: Pure C
- **Size**: 10% of LLVM code
- **Performance**: 70% of LLVM perf
- **Targets**: amd64, arm64, riscv64

### Technical Details

**Architecture**:
```
IR (SSA form) → QBE → Native assembly
```

**Features**:
- Full C ABI implementation
- Simple, hackable codebase
- ~10K-30K estimated binary size
- No external dependencies

### Integration Approach

```c
// kernel_lib/jit/qbe_wrapper.c
#include "qbe/all.h"

void* jit_compile_qbe(const char* ir_code) {
    // Parse QBE IL
    // Generate assembly
    // Assemble to machine code
    // Return function pointer
}
```

### Pros & Cons

✅ **Pros**:
- **Tiny size**: Estimated 30KB-100KB
- Pure C (easy bare-metal integration)
- Simple codebase (easy to understand)
- Full C ABI
- Actively maintained

❌ **Cons**:
- 30% slower than LLVM (70% perf)
- Different IR format (not LLVM IR)
- Less optimization passes
- Need to rewrite Phase 3 tests

### Estimated Effort

- IR format conversion: 2-3 days
- Integration: 2-3 days
- Testing: 2 days
- Bare-metal port: 2-3 days
- **Total**: ~8-11 days

---

## 🔬 Option 3: Cranelift Code Generator

### Overview

**Cranelift** = Fast, embeddable code generator
- **Language**: Rust
- **Size**: 200K LOC vs 20M for LLVM (100× smaller)
- **Performance**: 10× faster compilation, reasonable runtime
- **Targets**: x86_64, aarch64, riscv64

### Technical Details

**Features**:
- `no_std` support (bare-metal ready)
- No floating-point in compiler itself
- Efficient memory use
- Symbol lookup via dlsym (C FFI)

**Size estimate**: 1-3MB compiled

### Integration Approach

```rust
// kernel_lib/jit/cranelift_wrapper.rs
use cranelift_jit::{JITBuilder, JITModule};

#[no_mangle]
pub extern "C" fn jit_compile_cranelift(ir: *const u8, len: usize) -> *const () {
    // Parse Cranelift IR
    // JIT compile
    // Return function pointer
}
```

### Pros & Cons

✅ **Pros**:
- Small size (~1-3MB)
- `no_std` support
- Fast compilation (10× vs LLVM)
- Active development (WebAssembly focus)
- Good C FFI

❌ **Cons**:
- Rust dependency (complexity)
- Different IR format
- Less mature than LLVM
- Need Rust toolchain for bare-metal
- Learning curve

### Estimated Effort

- Rust bare-metal setup: 3-4 days
- Cranelift integration: 3-4 days
- IR conversion: 2-3 days
- Testing: 2 days
- **Total**: ~10-13 days

---

## 📊 Comparison Matrix

| Critère | LLVM Minimal | QBE | Cranelift |
|---------|-------------|-----|-----------|
| **Size** | 10-20MB ❌ | 30-100KB ✅✅ | 1-3MB ✅ |
| **Performance** | Baseline (1.17×) ✅ | 30% slower ⚠️ | Similar to LLVM ✅ |
| **Bare-metal** | Difficult ❌ | Easy (C) ✅ | Medium (Rust) ⚠️ |
| **C Integration** | Complex (C++) ❌ | Native (C) ✅ | Good (FFI) ✅ |
| **IR Compatibility** | LLVM IR ✅ | Custom IL ❌ | Custom IR ❌ |
| **Maturity** | Very high ✅ | Medium ⚠️ | High (WASM) ✅ |
| **Effort** | 9-10 days | 8-11 days | 10-13 days |
| **Risk** | Size issue | Perf regression | Complexity |

---

## 🎯 Recommendation

### Primary Choice: **QBE**

**Rationale**:
1. **Size target met**: 30-100KB << 5MB ✅
2. **Bare-metal ready**: Pure C, no dependencies ✅
3. **Acceptable perf**: 70% of LLVM = ~1.7× vs AOT (still <2×) ✅
4. **Simplicity**: Easy to understand and integrate ✅
5. **Hackable**: Can optimize hot paths ourselves ✅

**Trade-off**: 30% performance loss acceptable given:
- Still faster than interpreter (399× from Phase 3.3)
- Size reduction: 118MB → <1MB (>100× smaller!)
- Easier bare-metal integration
- Simpler codebase to maintain

### Fallback: **Cranelift**

If QBE performance insufficient:
- Still meets size target (1-3MB)
- Better performance (≈LLVM)
- `no_std` support
- More complexity acceptable if needed

### Not Recommended: **LLVM Minimal**

- Unlikely to meet <5MB target
- Complex bare-metal port
- Overkill for our use case

---

## 🚀 Next Steps (Session 23-24)

### Week 1: QBE Proof of Concept

**Day 1-2** (Now):
1. ⬜ Clone QBE repository
2. ⬜ Build QBE locally
3. ⬜ Measure binary size
4. ⬜ Test with simple C function

**Day 3-4**:
1. ⬜ Create QBE IL for fibonacci
2. ⬜ Compile and execute
3. ⬜ Measure performance vs Phase 3 baseline
4. ⬜ Validate acceptable perf (<2× vs AOT)

**Day 5**:
1. ⬜ Create integration plan
2. ⬜ Update ROADMAP.md
3. ⬜ Decision checkpoint

### Success Criteria

- [ ] QBE compiles successfully
- [ ] Binary size <500KB
- [ ] Fibonacci test <2× slower than AOT
- [ ] Integration path clear

If QBE fails any criterion → Evaluate Cranelift

---

## 📝 Build Scripts

### QBE Build Script

```bash
#!/bin/bash
# scripts/build_qbe.sh

set -e

echo "🔨 Building QBE Backend..."

# Clone QBE
if [ ! -d "qbe" ]; then
    git clone https://c9x.me/git/qbe.git
    cd qbe
else
    cd qbe
    git pull
fi

# Build
make clean
make

# Measure
echo ""
echo "📊 QBE Binary Size:"
ls -lh qbe
du -sh qbe

# Test
echo ""
echo "✅ Running QBE tests:"
make test

echo ""
echo "✅ QBE built successfully!"
```

### LLVM Minimal Script (fallback)

```bash
#!/bin/bash
# scripts/build_llvm_minimal.sh

set -e

echo "🔨 Building Minimal LLVM..."

if [ ! -d "llvm-project" ]; then
    git clone --depth 1 --branch release/18.x \
        https://github.com/llvm/llvm-project.git
fi

cd llvm-project
mkdir -p build-minimal
cd build-minimal

cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_BUILD_TOOLS=OFF \
  -DLLVM_INCLUDE_TOOLS=OFF \
  -DLLVM_ENABLE_THREADS=OFF \
  ../llvm

ninja

echo ""
echo "📊 LLVM Size:"
du -sh lib/libLLVM*.a

echo ""
echo "✅ LLVM built!"
```

---

## 📚 References

### QBE
- Homepage: https://c9x.me/compile/
- Git: https://c9x.me/git/qbe.git
- Docs: https://c9x.me/compile/doc/il.html

### Cranelift
- Homepage: https://cranelift.dev/
- GitHub: https://github.com/bytecodealliance/cranelift
- Docs: https://docs.rs/cranelift

### LLVM
- CMake Docs: https://llvm.org/docs/CMake.html
- Building Guide: https://llvm.org/docs/BuildingADistribution.html
- MinSizeRel: https://stackoverflow.com/questions/78502767/

---

**Decision**: QBE selected as primary path 🎯
**Next**: Build QBE proof of concept
**Created**: 2025-10-26
**Maintainer**: Claude Code Assistant