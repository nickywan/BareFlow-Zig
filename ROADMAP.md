# BareFlow - Project Roadmap

**Last Updated**: 2025-10-26 (Post-Session 28)
**Architecture**: Self-Optimizing Unikernel with "Grow to Shrink" Strategy
**Current Phase**: Phase 4 - Bare-Metal JIT Integration (Sessions 23-28 Complete - Testing Phase)

---

## 🎯 Vision: "Grow to Shrink" Strategy

**BareFlow = Programme Auto-Optimisant**

> "Le programme se suffit à lui-même, embarque tout au départ (60MB avec LLVM),
> s'auto-profile, s'auto-optimise, et converge vers un binaire minimal (2-5MB)."

### Progressive Convergence Lifecycle

```
Boot 1-10:   [60-118MB] Full LLVM + app in IR  → Interpreted (slow, profiles everything)
Boot 10-100: [30-60MB]  Hot paths JIT O0→O3    → 10× faster
Boot 100-500:[10MB]     Dead code eliminated    → Relinked optimized
Boot 500+:   [2-5MB]    Pure native export      → LLVM removed, appliance mode
```

**Inspirations**: PyPy warmup snapshots + LuaJIT tiered compilation + V8 PGO + 68000 self-contained programs

---

## ✅ Completed Phases

### Phase 1-2: AOT Baseline Unikernel (Sessions 17-20)
**Status**: ✅ **COMPLETE**

- **Session 17**: Architecture decision (monolithic → unikernel)
- **Session 18**: Runtime library extraction (kernel_lib.a 15KB)
- **Session 19**: TinyLlama application (13KB)
- **Session 20**: Boot validation and testing

**Results**:
- Binary size: 28KB (92% reduction from 346KB)
- Performance: 24 cycles/call overhead
- Boot time: ~3.1M cycles
- Zero syscall overhead achieved

### Phase 3: Hybrid Self-Optimizing Runtime (Sessions 21-22)
**Status**: ✅ **COMPLETE IN USERSPACE**

#### Phase 3.1: LLVM JIT Verification ✅
- Validated LLVM 18.1.8 OrcJIT
- Measured: 31KB binary + 118MB libLLVM.so
- Decision: Dynamic linking for dev, custom build for production

#### Phase 3.2: Static Linking Research ✅
- Tested: 27MB minimal (invalid), 110MB full (invalid)
- Root cause: Ubuntu LLVM requires dynamic linking
- Strategy: Hybrid approach confirmed

#### Phase 3.3: Interpreter vs JIT (Session 21) ✅
- AOT baseline: 0.028ms
- Interpreter: 13.9ms (498× slower)
- JIT: 0.035ms (1.25× vs AOT)
- **Result: 399× speedup validated!**

#### Phase 3.4: Tiered Compilation (Session 22) ✅
- Automatic: O0→O1→O2→O3 based on call counts
- Thresholds: 100, 1000, 10000 calls
- Overhead: 0.007% (negligible)
- Performance: 1.17× vs AOT (acceptable)

#### Phase 3.5: Dead Code Analysis (Session 22) ✅
- Total LLVM symbols: 32,451
- Used: 54 (0.17%)
- **Dead code: 99.83%**
- Size reduction potential: 95-98%

#### Phase 3.6: Native Export (Session 22) ✅
- JIT system: 118MB
- Native snapshot: 20KB
- **Size reduction: 99.98% (6000× smaller!)**
- Performance: Same as JIT O3

**Validation**: "Grow to Shrink" strategy proven end-to-end in userspace!

---

## 🚀 Current Phase: Phase 4 - Bare-Metal JIT Integration

### Session 23: FULL LLVM 18 Integration
**Status**: ✅ COMPLETE

**Goals**:
- [x] Use COMPLETE LLVM 18 (118MB - this is DESIRED!)
- [x] Verify ALL optimization passes available (O0→O3)
- [x] Keep Interpreter + OrcJIT + all features
- [x] NO size constraints - focus on auto-optimization capability

**Tasks**:
1. ✅ Correct project philosophy documentation
2. ✅ Install/verify FULL LLVM 18 (545 MB total, 118 MB main lib)
3. ✅ Confirm all optimization passes available
4. ✅ Test with Phase 3 validation suite (383× speedup confirmed)
5. ✅ Document bare-metal integration requirements

**Results**:
- LLVM 18.1.8 validated (545 MB full installation)
- 220 components available (all optimization passes O0-O3)
- Phase 3 tests pass: 383× interpreter→JIT speedup
- Bare-metal requirements documented (docs/phase4/)

### Session 24: C++ Runtime Implementation
**Status**: ✅ COMPLETE

**Goals**:
- [x] Implement bare-metal C++ runtime
- [x] operator new/delete using custom malloc
- [x] No C++ exceptions (-fno-exceptions)
- [x] No RTTI (-fno-rtti)

**Tasks**:
1. ✅ Create kernel_lib/cpp_runtime/ directory
2. ✅ Implement operator new/delete
3. ✅ Stub C++ exceptions
4. ✅ Stub RTTI and atexit functions
5. ✅ Build and test with custom malloc

**Results**:
- C++ runtime library: 6.7 KB
- All C++ features working (new/delete, virtual functions, RAII)
- Tests pass with custom malloc
- Zero standard library dependencies

### Session 25: Enhanced Allocator & System Stubs
**Status**: ✅ COMPLETE

**Goals**:
- [x] Enhance malloc for LLVM needs (200 MB heap)
- [x] Implement free-list allocator with proper free()
- [x] System call stubs (30+ functions)
- [x] Test with LLVM initialization

**Tasks**:
1. ✅ Implement free-list allocator (malloc_llvm.c, 390 lines)
2. ✅ 200 MB heap support (configurable)
3. ✅ System stubs (fprintf, abort, pthread, etc.)
4. ✅ Test allocator thoroughly (9/9 tests passed)
5. ✅ Test LLVM initialization (add(21,21) = 42)

**Results**:
- Free-list allocator: 390 lines, ~8 KB
- Heap: 200 MB (32 MB for testing)
- System stubs: 30+ functions, 5.3 KB
- C++ runtime: 12 KB total (with stubs)
- LLVM peak usage: 1.10 MB
- All tests: ✅ PASSED

### Session 26: Bare-Metal Integration
**Status**: ✅ COMPLETE

**Goals**:
- [x] Integrate malloc_llvm into kernel_lib
- [x] Create unified library (kernel_lib_llvm.a)
- [x] Build and test in 32-bit mode
- [x] Verify all LLVM symbols

**Tasks**:
1. ✅ Make malloc_llvm bare-metal compatible
2. ✅ Create Makefile.llvm for extended library
3. ✅ Build kernel_lib_llvm.a (23 KB)
4. ✅ Create pure bare-metal test (no stdlib)
5. ✅ Verify all 96 symbols exported

**Results**:
- kernel_lib_llvm.a: 23 KB (unified runtime)
- Bare-metal test: 5/5 passed (18 KB binary)
- 32-bit mode: ✅ Working
- Zero stdlib dependencies: ✅ Verified
- All LLVM symbols: ✅ Present (96 total)

### Session 27: LLVM Integration Strategy & Analysis
**Status**: ✅ COMPLETE

**Goals**:
- [x] Analyze current LLVM integration state
- [x] Identify architecture challenges (32-bit vs 64-bit)
- [x] Define boot integration strategy
- [x] Create action plan for Phase 4 completion

**Tasks**:
1. ✅ Assess Sessions 23-26 achievements
2. ✅ Identify 32-bit vs 64-bit mismatch issue
3. ✅ Analyze boot image requirements
4. ✅ Define solutions for each challenge
5. ✅ Create strategy for Sessions 28-30

**Results**:
- LLVM integration: ✅ Proven working (Session 25)
- Bare-metal runtime: ✅ Ready (Session 26, 23 KB)
- Challenge identified: 32-bit LLVM build needed
- Solution: Hybrid approach (64-bit dev, 32-bit later)
- Strategy: Complete Phase 4 with validation & docs

### Session 28: Enhanced LLVM Integration Tests
**Status**: ✅ COMPLETE

**Goals**:
- [x] Enhanced LLVM integration tests
- [x] Tiered compilation (O0→O3)
- [x] Test larger IR modules
- [x] Measure performance characteristics

**Tasks**:
1. ✅ Create tiered compilation test (O0→O3)
2. ✅ Test larger IR modules (3 functions: fib, factorial, sum_array)
3. ✅ Measure performance characteristics
4. ✅ Validate QEMU bare-metal environment

**Results**:
- test_tiered_llvm.cpp: 3 functions, 4 opt levels
- Performance: 1.7× speedup (O0 → O1)
- Compile time: 0.34-0.56 ms
- Execution time: 0.027-0.046 ms
- QEMU validation: All 4/4 tests passed

### Session 29-30: Boot Simulation & Documentation
**Status**: 📝 NEXT

**Goals**:
- [ ] Boot simulation in userspace
- [ ] Complete Phase 4 documentation
- [ ] Prepare for Phase 5 (TinyLlama)

**Tasks** (Session 29):
1. Create boot simulator
2. Measure initialization timing
3. Test interpreter + JIT workflow
4. Document boot sequence

**Tasks** (Session 30):
1. Complete Phase 4 summary
2. Create 32-bit LLVM build guide
3. Design Phase 5 TinyLlama plan
4. Update all documentation

### Session 29-30: Persistence
**Status**: 🔄 PLANNED

**Goals**:
- [ ] Save optimized code to FAT16
- [ ] Load snapshots on next boot
- [ ] Track optimization state
- [ ] Implement convergence detection

**Tasks**:
1. FAT16 write support
2. Snapshot format design
3. Code serialization
4. Snapshot loading at boot
5. Version management

---

## 📊 Phase 5: TinyLlama Model Integration

### Session 31-33: Model Loading
- [ ] Design weight format (.bin)
- [ ] Implement loader in bare-metal
- [ ] Load TinyLlama weights (~60MB)
- [ ] Create inference skeleton
- [ ] Profile layer execution

### Session 34-36: Inference Optimization
- [ ] Matrix multiply specialization
- [ ] Vectorization (SSE2/AVX2 detection)
- [ ] Memory layout optimization
- [ ] Quantization (int8/int4)
- [ ] Benchmark token/second

### Session 37-39: Self-Optimization
- [ ] Auto-detect hot layers
- [ ] JIT compile critical paths
- [ ] Specialize for weight shapes
- [ ] Cross-layer optimization
- [ ] Converge to optimal code

---

## 🎯 Success Metrics

### Phase 4 (Bare-Metal JIT)
- [ ] FULL LLVM 18 integrated (118MB is OK!)
- [ ] Boot 1: ~118MB image, profile everything
- [ ] Interpreter works bare-metal on all code
- [ ] JIT compilation successful (OrcJIT)
- [ ] 399× speedup demonstrated (Interpreter → JIT)
- [ ] Boot 100: Converged to ~30MB
- [ ] Boot 1000+: Converged to ~2-5MB native

### Phase 5 (TinyLlama)
- [ ] Model loads successfully
- [ ] Inference ≤1s per token
- [ ] Memory ≤256MB total
- [ ] Optimization convergence ≤500 boots
- [ ] Final binary ≤5MB

### Ultimate Goal
- [ ] Self-contained AI appliance
- [ ] No external dependencies
- [ ] Optimal for specific hardware
- [ ] Zero manual optimization
- [ ] Persistent improvements

---

## 📅 Timeline Estimates

| Phase | Sessions | Duration | Status |
|-------|----------|----------|---------|
| **Phase 1-2** (AOT Baseline) | 17-20 | ✅ Complete | Unikernel 28KB working |
| **Phase 3** (Userspace JIT) | 21-22 | ✅ Complete | 399× speedup validated |
| **Phase 4** (Bare-Metal JIT) | 23-30 | 2-3 weeks | 🚀 **CURRENT** (6/8 sessions) |
| **Phase 5** (TinyLlama) | 31-39 | 3-4 weeks | 📝 Planned |
| **Phase 6** (Production) | 40-50 | 4-6 weeks | 🔮 Future |

---

## 📈 Performance Evolution

### Current State (Phase 1-2)
```
Binary:      28KB (AOT-compiled)
Performance: Baseline
Flexibility: None
```

### Target State (Phase 4)
```
Binary:      60MB → 5MB (after convergence)
Performance: 10× faster on hot paths
Flexibility: Full JIT recompilation
```

### Ultimate State (Phase 5+)
```
Binary:      2-5MB (pure native, no LLVM)
Performance: Hardware-optimal
Flexibility: Re-enable JIT on new hardware
```

---

## 🔧 Technical Decisions

### 2025-10-25 (Session 17)
**Decision**: Pivot to unikernel architecture
**Rationale**: Monolithic kernel grew to 346KB with excessive complexity
**Result**: 92% size reduction, zero overhead

### 2025-10-26 (Session 21)
**Decision**: Validate with Interpreter first
**Rationale**: Prove 498× slowdown acceptable for universal profiling
**Result**: 399× speedup to JIT confirmed strategy

### 2025-10-26 (Session 22)
**Decision**: Complete all Phase 3 in userspace
**Rationale**: Validate entire strategy before bare-metal complexity
**Result**: End-to-end validation successful, 6000× size reduction proven

### Next Decision Point
**Question**: Custom LLVM build vs. Alternative JIT (QBE, MIR, Cranelift)?
**Factors**: Size, complexity, features, maintenance
**Timeline**: Session 23-24

---

## 📚 Key Documentation

### Architecture & Strategy
- `README.md` - Vision and "Grow to Shrink" philosophy
- `ARCHITECTURE_UNIKERNEL.md` - Detailed unikernel design
- `CLAUDE.md` - AI assistant context (updated)
- `CLAUDE_CONTEXT.md` - Session history and state

### Phase 3 Results (Validation)
- `PHASE3_2_FINDINGS.md` - Static linking research
- `PHASE3_3_RESULTS.md` - 399× speedup proof
- `PHASE3_4_TIERED_JIT.md` - Tiered compilation
- `PHASE3_5_DCE_RESULTS.md` - 99.83% dead code
- `PHASE3_6_NATIVE_EXPORT.md` - 6000× reduction

### Implementation
- `kernel_lib/` - Runtime library (15KB)
- `tinyllama/` - Unikernel application (13KB)
- `test_*.cpp` - Phase 3 validation programs
- `analyze_llvm_usage.sh` - Dead code analysis tool

---

## ⚠️ Risks & Mitigations

### Risk 1: LLVM too large for bare-metal
**Mitigation**: Custom minimal build, alternative JITs (QBE, Cranelift)

### Risk 2: C++ runtime dependencies
**Mitigation**: Pure C interface, custom allocator, no exceptions

### Risk 3: Convergence takes too long
**Mitigation**: Persistent snapshots, profile sharing, pre-optimization

### Risk 4: Performance regression
**Mitigation**: Continuous profiling, A/B testing, rollback capability

---

## 🎉 Achievements So Far

1. **92% size reduction** (346KB → 28KB)
2. **399× speedup proven** (Interpreter → JIT)
3. **99.83% dead code identified** in LLVM
4. **6000× size reduction** demonstrated
5. **Zero syscall overhead** achieved
6. **Tiered compilation** working (O0→O3)
7. **Native export** validated
8. **"Grow to Shrink"** strategy proven end-to-end

---

**Project**: BareFlow - Self-Optimizing Unikernel
**Maintainer**: Claude Code Assistant
**Human**: @nickywan
**Status**: 🚀 Phase 4 Starting - Bare-Metal JIT Integration