# Next Session - Phase 3.5: Dead Code Elimination

**Date**: 2025-10-26
**Branch**: `feat/true-jit-unikernel`
**Status**: Phase 3.4 COMPLETE, ready for Phase 3.5 or decision point

---

## 🎯 What We Achieved (Phase 3.1-3.4)

### ✅ Phase 3.1: LLVM JIT Verification
- LLVM 18.1.8 installation verified
- `test_jit_minimal.cpp` working (31KB + 118MB .so)
- JIT compilation validated in userspace

### ✅ Phase 3.2: Static Linking Research
- Tested minimal static (30 libs): 27MB - invalid
- Tested full static (all libs): 110MB - invalid
- **Decision**: Use dynamic linking for development (Option B)
- Documented in `PHASE3_2_FINDINGS.md`

### ✅ Phase 3.3: Interpreter vs JIT Comparison
- Created `test_llvm_interpreter.cpp`
- **Results**:
  - AOT (clang -O2): 0.028 ms (baseline)
  - Interpreter: 13.9 ms (498× slower)
  - JIT: 0.035 ms (1.25× slower than AOT, **399× faster than Interpreter!**)
- **"Grow to Shrink" strategy VALIDATED** ✅
- Documented in `PHASE3_3_RESULTS.md`

### ✅ Phase 3.4: Tiered JIT Compilation
- Created `test_tiered_jit.cpp`
- **Automatic recompilation** at thresholds (100, 1000, 10000)
- **Results**:
  - O0: 6.31 ms compile, ~4.20 ms exec (calls 1-99)
  - O1: 2.65 ms compile, ~4.02 ms exec (calls 100-999)
  - O2: 2.77 ms compile, ~4.03 ms exec (calls 1000-9999)
  - O3: 2.42 ms compile, ~4.04 ms exec (calls 10000+)
  - **JIT O3 vs AOT**: 1.17× slower (acceptable!)
  - **Compilation overhead**: 0.007% of total time
- **Tiered compilation VALIDATED** ✅
- Documented in `PHASE3_4_TIERED_JIT.md`

---

## 🚦 Decision Point: What's Next?

**User requested pause after Phase 3** to evaluate next steps.

You now have **three validated approaches**:
1. ✅ Interpreter mode (slow, universal profiling)
2. ✅ JIT mode (near-AOT performance)
3. ✅ Tiered compilation (automatic optimization)

### Option A: Continue to Phase 3.5-3.6 (Userspace)

**Phase 3.5: Dead Code Elimination** (1-2 sessions)
- Track which LLVM components are actually used
- Identify dead code (40-60% of LLVM expected)
- Custom linker script to exclude unused symbols
- Expected reduction: 60MB → 10-20MB

**Phase 3.6: Native Export** (1-2 sessions)
- Extract JIT-compiled machine code
- Link into standalone binary
- Persistent snapshot to filesystem
- Final binary: 2-5MB pure native

**Benefits**:
- ✅ Complete "Grow to Shrink" validation in userspace
- ✅ Understand full optimization cycle before bare-metal
- ✅ Measure actual size reductions
- ✅ Prove concept end-to-end

### Option B: Port to Bare-Metal Now

**Integrate with BareFlow Unikernel**
- Add LLVM runtime to kernel (60MB initial)
- Implement tiered JIT in bare-metal
- Profile actual TinyLlama workload
- See real behavior early

**Benefits**:
- ✅ Real hardware profiling data
- ✅ Identify bare-metal challenges early
- ✅ Working demo sooner
- ⚠️ More complex debugging

### Option C: Pause and Architect

**Deep dive into production architecture**
- Plan memory management for JIT
- Design snapshot format
- Plan filesystem integration
- Create comprehensive roadmap

**Benefits**:
- ✅ Clear plan before major work
- ✅ Avoid rework
- ✅ Better understanding of trade-offs

---

## 📊 Current State Summary

### What Works (Validated in Userspace)
- ✅ LLVM 18.1.8 JIT compilation
- ✅ Interpreter mode (498× slower, great for profiling)
- ✅ JIT mode (1.17× slower than AOT, near-optimal)
- ✅ Tiered compilation (O0→O1→O2→O3 automatic)
- ✅ Profiling (call counts, execution time)
- ✅ Recompilation (automatic at thresholds)

### What's Pending
- ⚠️ Dead code elimination (Phase 3.5)
- ⚠️ Native code export (Phase 3.6)
- ⚠️ Bare-metal integration
- ⚠️ Persistent optimization
- ⚠️ TinyLlama model integration

### Performance Validated
- **Binary size**: 49KB (test executable)
- **Dynamic lib**: 118MB (libLLVM-18.so)
- **Compilation overhead**: 0.007% (negligible)
- **JIT vs AOT**: 1.17× slower (acceptable)
- **Interpreter vs JIT**: 399× speedup

---

## 🚀 Recommended Next Step: Phase 3.5

### Why Phase 3.5 First?

**Rationale**:
1. **Validate size reduction**: Prove 60MB → 10-20MB is achievable
2. **Low risk**: Userspace testing, easy to debug
3. **Complete the story**: See full "Grow to Shrink" cycle
4. **Inform bare-metal**: Know what to include/exclude

### Phase 3.5 Implementation Plan

**Step 1: Function Coverage Tracking** (30 min)
```cpp
struct FunctionCoverage {
    std::set<std::string> called_functions;

    void recordCall(const std::string& name) {
        called_functions.insert(name);
    }

    void exportCoverage(const char* filename) {
        // Write to JSON: {"called": ["fibonacci", ...]}
    }
};
```

**Step 2: LLVM Component Analysis** (1-2 hours)
- Run `ldd test_tiered_jit` to see dependencies
- Use `nm -gC libLLVM-18.so` to list all symbols
- Track which symbols are actually used (via profiling)
- Generate "used" vs "total" report

**Step 3: Custom Linking** (2-3 hours)
- Create linker script excluding dead symbols
- Build with `-Wl,--gc-sections` (garbage collection)
- Strip unused LLVM components
- Measure final binary size

**Expected Output**:
```
Original binary:    49KB
Original libLLVM:   118MB
After DCE binary:   30-40KB
After DCE libLLVM:  20-40MB (50-66% reduction)
```

**Step 4: Documentation**
- Create `PHASE3_5_DCE_RESULTS.md`
- Update `JIT_ANALYSIS.md`
- Update `CLAUDE_CONTEXT.md`

---

## 📝 Alternative: Quick Wins First

If you want **faster results** before Phase 3.5, consider:

### Quick Win 1: Test with Different Functions (30 min)
- Add matrix multiply to `test_tiered_jit.cpp`
- See if O3 makes a bigger difference (should!)
- Validate JIT specialization

### Quick Win 2: Measure Interpreter Baseline (1 hour)
- Add Interpreter mode to `test_tiered_jit.cpp`
- Start in Interpreter, transition to JIT at threshold
- Demonstrate full Interpreter→O0→O1→O2→O3 pipeline

### Quick Win 3: Profile Data Export (1 hour)
- Export profiling data to JSON
- Create visualization script (Python/gnuplot)
- Show performance evolution over time

---

## 📂 Files to Review

### Phase 3.4 Outputs
- `test_tiered_jit.cpp` - Tiered JIT implementation
- `Makefile.tiered` - Build system
- `PHASE3_4_TIERED_JIT.md` - Complete results and analysis

### Previous Phases
- `PHASE3_3_RESULTS.md` - Interpreter vs JIT
- `PHASE3_2_FINDINGS.md` - Static linking research
- `JIT_ANALYSIS.md` - Overall strategy

### Project Documentation
- `ROADMAP.md` - Project roadmap (needs update)
- `CLAUDE_CONTEXT.md` - Current state (needs update)
- `ARCHITECTURE_UNIKERNEL.md` - Unikernel architecture

---

## 🎯 Success Criteria (If Continuing to Phase 3.5)

### Minimum Requirements
- ✅ Track function call coverage
- ✅ Identify unused LLVM components
- ✅ Measure size reduction (target: 50%+ of libLLVM)
- ✅ Verify functionality after DCE

### Stretch Goals
- ✅ Custom LLVM build with minimal components
- ✅ Static linking with reduced LLVM
- ✅ Full dependency tree analysis
- ✅ Automated dead code elimination tool

---

## 💡 User Choice Required

**Please decide**:

1. **Continue to Phase 3.5** (Dead Code Elimination in userspace)
   - Complete "Grow to Shrink" validation
   - 1-2 sessions estimated

2. **Port to bare-metal now** (integrate with BareFlow kernel)
   - See real behavior early
   - More complex but exciting

3. **Quick wins first** (test more functions, visualizations)
   - Build confidence
   - 1-2 hours each

4. **Pause and architect** (plan production system)
   - Create comprehensive roadmap
   - Design decisions first

**What would you like to do next?**

---

**Created**: 2025-10-26
**Status**: Phase 3.4 complete, awaiting user decision
**Estimated Time**: Phase 3.5 = 1-2 sessions (3-6 hours)
