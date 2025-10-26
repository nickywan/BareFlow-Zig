# Next Session - Phase 3.4: Tiered JIT Compilation

**Date**: 2025-10-26
**Branch**: `feat/true-jit-unikernel`
**Status**: Phase 3.3 COMPLETE, ready for Phase 3.4

---

## 🎯 What We Achieved (Phase 3.1-3.3)

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

---

## 🚀 Next Phase: 3.4 - Tiered JIT Compilation

### Goal
Implement **multi-level JIT optimization** with automatic recompilation based on call count thresholds.

### Why Tiered Compilation?

Phase 3.3 proved:
- JIT reaches AOT performance (1.25× overhead)
- Interpreter → JIT gives **399× speedup**

But compilation is expensive:
- O0: Fast compile (~10ms), slower runtime
- O3: Slow compile (~500ms), optimal runtime

**Solution**: Start with O0, upgrade to O3 only for **hot** functions.

---

## 📋 Implementation Plan

### Step 1: Extend `test_llvm_interpreter.cpp` (1-2h)

Add tiered JIT with multiple optimization levels:

```cpp
enum OptLevel {
    OPT_INTERPRETER = -1,  // Not compiled (interpreted)
    OPT_O0 = 0,            // Fast compile, basic codegen
    OPT_O1 = 1,            // Balanced
    OPT_O2 = 2,            // Aggressive
    OPT_O3 = 3             // Maximum optimization
};

struct TieredFunction {
    std::string name;
    void* code_ptr;
    OptLevel current_level;
    uint64_t call_count;
    uint64_t total_cycles;

    // Thresholds
    static const int WARM_THRESHOLD = 100;      // O0
    static const int HOT_THRESHOLD = 1000;      // O2
    static const int VERY_HOT_THRESHOLD = 10000; // O3
};
```

### Step 2: Implement Recompilation Logic

```cpp
void recordCall(TieredFunction& func, uint64_t cycles) {
    func.call_count++;
    func.total_cycles += cycles;

    // Check for upgrade
    if (func.call_count == TieredFunction::WARM_THRESHOLD) {
        recompile(func, OPT_O0);
    } else if (func.call_count == TieredFunction::HOT_THRESHOLD) {
        recompile(func, OPT_O2);
    } else if (func.call_count == TieredFunction::VERY_HOT_THRESHOLD) {
        recompile(func, OPT_O3);
    }
}
```

### Step 3: Test with Realistic Workload

Run `fibonacci(30)` with **50,000 iterations** to trigger all thresholds.

**Expected Output**:
```
Iteration 100: Recompiled fibonacci: INTERPRETER → O0
Iteration 100: Level O0, avg 50000 cycles
Iteration 1000: Recompiled fibonacci: O0 → O2
Iteration 1000: Level O2, avg 1500 cycles
Iteration 10000: Recompiled fibonacci: O2 → O3
Iteration 10000: Level O3, avg 800 cycles
Final: avg 900 cycles (80× faster than interpreted!)
```

---

## 📊 Expected Performance

Based on Phase 3.3 results:

| Optimization | Compile Time | Execution Time | vs Interpreter |
|--------------|--------------|----------------|----------------|
| **Interpreter** | 0 ms | 13.9 ms | 1× (baseline) |
| **O0** | ~5-10 ms | ~2 ms | ~7× faster |
| **O1** | ~20-50 ms | ~0.5 ms | ~30× faster |
| **O2** | ~100-200 ms | ~0.05 ms | ~280× faster |
| **O3** | ~500+ ms | ~0.035 ms | **399× faster** |

**Insight**:
- First 100 calls: Interpreter (slow, but profile gathering)
- Calls 100-1000: O0 (fast compile, decent speed)
- Calls 1000-10000: O2 (production speed)
- Calls 10000+: O3 (fully optimized)

---

## 🎯 Success Criteria

### Minimum Requirements (1 session)
- ✅ `test_tiered_jit.cpp` compiles and runs
- ✅ Automatic recompilation at thresholds (100, 1000, 10000)
- ✅ Visible speedup: Interpreter → O0 → O2 → O3
- ✅ Final O3 matches Phase 3.3 JIT performance (~0.035 ms)

### Stretch Goals (if time permits)
- ✅ Add compilation time tracking
- ✅ Graph: call_count vs avg_cycles (show step changes)
- ✅ Test with multiple functions (identify hot vs cold)

---

## 📂 Files to Create/Modify

### New Files
- `test_tiered_jit.cpp` - Tiered JIT implementation
- `Makefile.tiered` - Build system

### Documentation
- `PHASE3_4_TIERED_JIT.md` - Results and analysis
- Update `JIT_ANALYSIS.md` - Add tiered compilation section
- Update `CLAUDE_CONTEXT.md` - Mark Phase 3.4 complete

---

## 🚦 Decision Point After Phase 3.4

**User requested pause after Phase 3** to evaluate:

### Option A: Continue to Phase 3.5-3.6 (Userspace)
- Phase 3.5: Dead code elimination
- Phase 3.6: Native export
- **Benefits**: Complete userspace validation

### Option B: Port to Bare-Metal Now
- Integrate tiered JIT with kernel
- Boot with 60MB LLVM runtime
- **Benefits**: See real behavior early

### Option C: Pause and Plan
- Document architecture decisions
- Plan full bare-metal roadmap
- **Benefits**: Clear plan before committing

---

## 📝 Notes for Next Session

### Context to Remember
1. **LLVM Version**: 18.1.8 (dynamic linking)
2. **Strategy**: "Grow to Shrink" validated ✅
3. **Key Finding**: JIT = 1.25× AOT, 399× Interpreter
4. **Philosophy**: Size doesn't matter at boot 1 (60MB OK)

### Commands to Resume
```bash
cd /home/nickywan/dev/Git/BareFlow-LLVM
git status
cat PHASE3_3_RESULTS.md  # Review results
make -f Makefile.interpreter run  # Re-run if needed
```

---

**Created**: 2025-10-26
**Status**: Ready for Phase 3.4 or decision point
**Estimated Time**: 2-4 hours for tiered JIT
