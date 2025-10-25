# NEXT SESSION - Session 20

**Date**: 2025-10-26
**Branch**: `claude/merge-interface-runtime-011CUMDiW4omhPaJemQSVuoR`
**Previous**: Sessions 18-19 (kernel_lib.a + tinyllama unikernel)

## 📋 Session 20 Objectives

**Goal**: Test, validate and benchmark the new unikernel architecture

**Duration Estimate**: 4-5 hours

---

## Phase 1: Boot Testing & Validation (1h)

### Task 1.1: Test Complete Boot Sequence
```bash
cd tinyllama
make run  # Should boot and display VGA output + serial
```

**Expected Output**:
- VGA: Green header "TinyLlama Unikernel v0.1"
- 3 test sections: fibonacci, sum_to_n, count_primes
- Each test shows: iterations, first result, profiling stats
- Final summary with all stats

**Validation**:
- [ ] Boot completes without crashes
- [ ] All 3 tests execute
- [ ] VGA colors correct (green header, yellow stats, white text)
- [ ] Serial output matches VGA

### Task 1.2: Capture and Analyze Serial Output
```bash
cd tinyllama
timeout 10 qemu-system-i386 -drive file=tinyllama.img,format=raw \
  -serial file:/tmp/tinyllama_test.txt -display none

cat /tmp/tinyllama_test.txt
```

**Check**:
- [ ] Serial init message appears
- [ ] Test messages logged
- [ ] No corruption or garbage characters
- [ ] Complete output (not truncated)

### Task 1.3: Verify Profiling Data

**Expected Metrics** (approximate):
- fibonacci(10) × 10: ~50-100 cycles/call
- sum(1..1000) × 100: ~2000-5000 cycles/call
- primes(<100) × 5: ~10000-50000 cycles/call

**Validation**:
- [ ] Call counts correct (10, 100, 5)
- [ ] Cycle counts reasonable (not 0 or corrupted)
- [ ] Min ≤ Avg ≤ Max
- [ ] Total cycles = call_count × avg_cycles (approximately)

---

## Phase 2: Performance Benchmarking (1h)

### Task 2.1: Measure Boot Time

**Method**: Add timestamps in main.c

```c
void main(void) {
    uint64_t start = cpu_rdtsc();
    vga_init();
    serial_init();
    uint64_t init_done = cpu_rdtsc();

    // ... tests ...

    uint64_t end = cpu_rdtsc();
    serial_puts("[TIMING] Init: ");
    serial_put_uint64(init_done - start);
    serial_puts(" cycles\n[TIMING] Total: ");
    serial_put_uint64(end - start);
    serial_puts(" cycles\n");
}
```

**Measure**:
- [ ] Initialization time (vga_init + serial_init)
- [ ] Total execution time
- [ ] Time per test section

### Task 2.2: Binary Size Comparison

```bash
# Unikernel
ls -lh tinyllama/tinyllama_bare.bin
ls -lh kernel_lib/kernel_lib.a

# Old monolithic (for comparison)
ls -lh build/kernel.bin  # If it still exists
```

**Document**:
- [ ] TinyLlama binary: 13KB
- [ ] kernel_lib.a: 15KB
- [ ] Total: 28KB
- [ ] vs Old kernel: 346KB (if available)
- [ ] Reduction: 92%

### Task 2.3: Function Call Overhead

**Add microbenchmark to main.c**:

```c
// Direct call (unikernel)
static int dummy_function(int x) {
    return x + 1;
}

void benchmark_calls(void) {
    uint64_t start = cpu_rdtsc();
    volatile int result = 0;
    for (int i = 0; i < 10000; i++) {
        result = dummy_function(i);
    }
    uint64_t end = cpu_rdtsc();

    serial_puts("[BENCH] 10000 calls: ");
    serial_put_uint64(end - start);
    serial_puts(" cycles (");
    serial_put_uint((uint32_t)((end - start) / 10000));
    serial_puts(" cycles/call)\n");
}
```

**Expected**: ~2-5 cycles/call for direct calls

---

## Phase 3: Documentation & Reporting (1h)

### Task 3.1: Create PERFORMANCE_COMPARISON.md

```markdown
# Performance Comparison: Monolithic vs Unikernel

## Binary Size
| Metric | Monolithic (Session 17) | Unikernel (Session 19) | Improvement |
|--------|-------------------------|------------------------|-------------|
| Kernel | 346KB | 28KB | 92% reduction |
| Modules | 28 embedded | 0 | 100% simplification |

## Boot Time
- Initialization: X cycles
- Total execution: Y cycles
- Time to first test: Z cycles

## Function Call Overhead
- Direct call: ~2-5 cycles
- (vs syscall simulation: ~50-100 cycles - theoretical)

## Memory Footprint
- Heap: 256KB
- Stack: grows down from 0x90000
- Total: ~300KB runtime

## Conclusion
[Summary of improvements and trade-offs]
```

### Task 3.2: Update README.md

Update sections:
- [ ] Architecture diagram (show unikernel)
- [ ] Build instructions (point to tinyllama/)
- [ ] Quick start (cd tinyllama && make run)
- [ ] Remove old module system references

### Task 3.3: Create BUILD_GUIDE.md

Document:
- [ ] Prerequisites (clang-18, nasm, qemu)
- [ ] Building kernel_lib.a
- [ ] Building tinyllama application
- [ ] Creating bootable image
- [ ] Running in QEMU
- [ ] Debugging tips

---

## Phase 4: Commit & Push (30 min)

### Task 4.1: Commit Unikernel Implementation

```bash
git add tinyllama/ kernel_lib/ CLAUDE_CONTEXT.md NEXT_SESSION.md
git commit -m "feat(unikernel): Complete TinyLlama self-profiling application

Sessions 18-19:
- Extracted kernel_lib.a (15KB runtime library)
- Created tinyllama unikernel application (13KB)
- Implemented self-profiling demo (fibonacci, sum, primes)
- Total size: 28KB (92% reduction from 346KB)
- Bootable image: tinyllama.img (10MB)

Architecture:
- Zero syscall overhead (direct function calls)
- Self-optimization ready (jit_profile_* API)
- Single binary simplicity
- 100% code reusability"

git push origin claude/merge-interface-runtime-011CUMDiW4omhPaJemQSVuoR
```

### Task 4.2: Create Summary Document

Create `SESSION_19_SUMMARY.md` with:
- [ ] What was built
- [ ] Key achievements
- [ ] Performance metrics
- [ ] Next steps

---

## Phase 5: Next Steps Planning (30 min)

### Immediate (Week 3)
1. **Hot-Path Optimization**
   - Analyze cycle counts from profiling
   - Identify functions >1000 cycles/call
   - Apply manual optimizations (loop unrolling, inlining)
   - Re-measure performance

2. **Enhanced Profiling**
   - Add JSON export to serial output
   - Create profile visualization tool
   - Track performance across builds

### Medium Term (Weeks 4-5)
3. **TinyLlama Model Integration**
   - Design model format (.bin weights file)
   - Implement layer-by-layer inference
   - Add self-profiling to inference loop
   - Measure inference time

4. **Adaptive Optimization**
   - Implement hot-path detection
   - Add runtime recompilation triggers
   - Atomic code swapping
   - Verify zero-downtime optimization

### Long Term (Week 6+)
5. **Meta-Circular JIT (Phase 3)**
   - LLVM bitcode interpreter
   - Self-hosting compiler
   - Persistent optimization state

---

## Success Criteria

- [x] TinyLlama boots successfully
- [ ] All profiling stats displayed correctly
- [ ] Performance documented (boot time, call overhead)
- [ ] Comparison report created
- [ ] Documentation updated (README, BUILD_GUIDE)
- [ ] Code committed and pushed
- [ ] Next session plan created

---

## Troubleshooting

### If boot fails:
1. Check serial output for early messages
2. Verify FLUD signature in binary
3. Check bootloader capacity (128 sectors = 64KB)
4. Test with `make debug` for CPU logs

### If profiling shows zeros:
1. Verify rdtsc instruction works in QEMU
2. Check jit_profile_begin/end calls
3. Ensure profiler initialized
4. Add debug serial_puts in profile.c

### If serial corrupted:
1. Check baud rate (115200)
2. Verify serial_init() called before use
3. Test with simpler output first
4. Check for buffer overflows

---

## Time Estimates

| Phase | Tasks | Est. Time | Status |
|-------|-------|-----------|--------|
| 1 | Boot testing & validation | 1h | Pending |
| 2 | Performance benchmarking | 1h | Pending |
| 3 | Documentation & reporting | 1h | Pending |
| 4 | Commit & push | 30min | Pending |
| 5 | Next steps planning | 30min | Pending |
| **Total** | | **4h** | **0% complete** |

---

**Created**: 2025-10-26
**For**: Session 20 (Testing & Validation)
**Previous**: Sessions 18-19 (Unikernel Implementation - Complete)
