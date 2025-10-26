# BareFlow - Project Roadmap

**Last Updated**: 2025-10-26 (Post-Session 22)
**Architecture**: Self-Optimizing Unikernel with "Grow to Shrink" Strategy
**Current Phase**: Phase 4 - Bare-Metal JIT Integration (Starting)

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

### Session 23-24: FULL LLVM 18 Integration
**Status**: 📝 NEXT

**Goals**:
- [ ] Use COMPLETE LLVM 18 (118MB - this is DESIRED!)
- [ ] Verify ALL optimization passes available (O0→O3)
- [ ] Keep Interpreter + OrcJIT + all features
- [ ] NO size constraints - focus on auto-optimization capability

**Tasks**:
1. ✅ Correct project philosophy documentation
2. Install/verify FULL LLVM 18
3. Confirm all optimization passes available
4. Test with Phase 3 validation suite
5. Document bare-metal integration requirements

### Session 25-26: Bare-Metal Port
**Status**: 🔄 PLANNED

**Goals**:
- [ ] Port LLVM OrcJIT to bare-metal
- [ ] Custom allocator (no malloc)
- [ ] No C++ exceptions (-fno-exceptions)
- [ ] No RTTI (-fno-rtti)

**Tasks**:
1. Implement custom memory allocator
2. Stub out system dependencies
3. Create bare-metal JIT wrapper
4. Test with simple IR functions
5. Integrate with kernel_lib

### Session 27-28: Boot Integration
**Status**: 🔄 PLANNED

**Goals**:
- [ ] Create ~118MB bootable image with FULL LLVM (Boot 1)
- [ ] Boot and run interpreter on all code
- [ ] Profile ALL function calls universally
- [ ] JIT compile hot paths (O0→O3)

**Tasks**:
1. Link LLVM with tinyllama
2. Create large bootable image
3. Test interpreter execution
4. Implement profiling hooks
5. Trigger JIT compilation

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
| **Phase 4** (Bare-Metal JIT) | 23-30 | 2-3 weeks | 🚀 **CURRENT** |
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