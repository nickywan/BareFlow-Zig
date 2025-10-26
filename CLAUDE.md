# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## ⚠️ CRITICAL: Project Philosophy - READ FIRST

### 🎯 "Grow to Shrink" - Size is NOT a Constraint!

**NEVER OPTIMIZE FOR INITIAL SIZE! THE SYSTEM MUST BE LARGE AT START!**

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

### 🚫 NEVER Do This:
- ❌ Try to minimize LLVM size
- ❌ Use "minimal builds" or stripped versions
- ❌ Consider alternatives like QBE, Cranelift
- ❌ Optimize for boot image size
- ❌ Remove LLVM features to save space

### ✅ ALWAYS Do This:
- ✅ Use **FULL LLVM 18** (118MB is FINE!)
- ✅ Include ALL optimization passes
- ✅ Keep LLVM Interpreter + OrcJIT
- ✅ Focus on auto-optimization capability
- ✅ Size reduction comes FROM convergence, NOT initial constraints

**Why**: The system profiles itself, JIT compiles hot paths, eliminates dead code, and THEN exports to small native binary. Initial size is IRRELEVANT - convergence is everything!

---

## 📌 Session Recovery Protocol

**IMPORTANT**: When starting a new session, follow this exact sequence:

### 1. Read Core Documents (in this order)
```bash
1. CLAUDE.md           # This file - project instructions
2. ROADMAP.md         # Current phase, next steps, timeline
3. README.md          # Vision and strategy overview
```

### 2. Check Current State
```bash
# Check git status
git status
git branch --show-current
git log --oneline -5

# Check project structure
tree -d -L 2 || ls -la

# Check latest test results if Phase 3+
ls -la tests/phase3/*.cpp 2>/dev/null | head -5
```

### 3. Read Phase-Specific Documentation
Based on current phase in ROADMAP.md:
- **Phase 4 (Bare-Metal JIT)**: Read `docs/phase3/PHASE3_*.md` for validation results
- **Phase 5 (TinyLlama)**: Read `ARCHITECTURE_UNIKERNEL.md`
- **If debugging**: Read `BOOTLOADER_NOTES.md`

### 4. Update Session Context
After completing work in a session:
1. Update `ROADMAP.md` with completed tasks
2. Update this `CLAUDE.md` if commands or architecture changed
3. Create/update phase documentation in `docs/phaseX/`
4. Commit with detailed message including session number

---

## 📋 Document Maintenance Protocol

### Files to Keep Current
| File | Purpose | Update When |
|------|---------|-------------|
| **CLAUDE.md** | AI instructions | Commands or workflow changes |
| **ROADMAP.md** | Project timeline | Tasks completed or phases changed |
| **README.md** | Project vision | Major strategy changes only |

### Files to Archive
When files become obsolete, move to `archive/`:
```bash
mkdir -p archive/docs
mv OLD_FILE.md archive/docs/
git add -A && git commit -m "archive: Move obsolete docs"
```

### Session Documentation
For each major session:
1. Create `docs/phase{X}/SESSION_{Y}_RESULTS.md` if significant progress
2. Include: Goals, Actions, Results, Metrics, Next Steps
3. Keep concise (< 500 lines)

---

## 🏗️ Project Overview

**BareFlow** is a self-optimizing unikernel implementing the "Grow to Shrink" strategy - a hybrid AOT+JIT system that starts large (60MB with full LLVM), profiles itself, optimizes hot paths, eliminates dead code, and converges to a minimal native binary (2-5MB).

**Current State**: Phase 3 complete, Phase 4 starting (Bare-Metal JIT Integration)

**Architecture**: Unikernel design (28KB) with kernel_lib.a runtime (15KB) + application (13KB). No syscalls, direct function calls, Ring 0 execution.

### "Grow to Shrink" Lifecycle
```
Boot 1-10:   [60-118MB] Full LLVM + IR → Interpreted (profiles everything)
Boot 10-100: [30-60MB]  Hot paths JIT → 10× faster
Boot 100+:   [2-5MB]    Native export  → LLVM removed
```

---

## 🚀 Build Commands

### Bare-Metal Unikernel
```bash
# Build runtime library
cd kernel_lib && make         # → kernel_lib.a (15KB)

# Build and boot unikernel
cd tinyllama
make                          # Build everything
make run                      # Boot in QEMU with serial
make debug                    # Boot with CPU debug logs
```

### Phase 3 JIT Testing (Userspace Validation)
```bash
# Build and run specific test
cd tests/phase3
clang++-18 -g test_tiered_jit.cpp -o test_tiered_jit \
  $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native)
./test_tiered_jit

# Quick test all Phase 3
for test in test_llvm_interpreter test_tiered_jit test_native_export; do
    echo "Running $test..."
    ./$test | tail -5
done
```

### Cleanup & Organization
```bash
./cleanup_project.sh          # Archive old code, organize tests
find . -name "*.o" -delete    # Clean object files
```

---

## 📊 Key Metrics (Phase 3 Validated)

| Metric | Value | Significance |
|--------|-------|--------------|
| **Binary Size** | 28KB | 92% reduction from 346KB |
| **Interpreter→JIT** | 399× | Speedup validation |
| **Dead Code** | 99.83% | LLVM unused portions |
| **Native Export** | 6000× | Size reduction (118MB→20KB) |
| **Call Overhead** | 24 cycles | Direct calls, no syscalls |

---

## 🎯 Current Phase Status

### ✅ Phase 3: Hybrid Self-Optimizing Runtime (COMPLETE)
All validation done in userspace:
- Phase 3.1: LLVM JIT Verification ✅
- Phase 3.2: Static Linking Research ✅
- Phase 3.3: Interpreter vs JIT (399× speedup) ✅
- Phase 3.4: Tiered Compilation (O0→O3) ✅
- Phase 3.5: Dead Code Analysis (99.83%) ✅
- Phase 3.6: Native Export (6000× smaller) ✅

### 🚀 Phase 4: Bare-Metal JIT Integration (STARTING)
Next sessions (23-30):
- Custom LLVM build (2-5MB target)
- Bare-metal port (no exceptions, custom allocator)
- Boot integration (60MB image with LLVM)
- Persistence (FAT16 snapshots)

See `ROADMAP.md` for detailed task list.

---

## 💻 Development Workflow

### Starting Work on a Task
1. Check current phase in `ROADMAP.md`
2. Find next unchecked task
3. Read relevant phase docs in `docs/phaseX/`
4. Implement and test
5. Update documentation
6. Commit with clear message

### Adding New Features
1. **Unikernel App**: Edit `tinyllama/main.c`
2. **Runtime Library**: Edit `kernel_lib/{io,memory,cpu,jit}/`
3. **Build System**: Update relevant Makefile
4. **Test First**: Create test in `tests/phaseX/`

### Testing Protocol
1. **Userspace First**: Validate concept with test_*.cpp
2. **Port to C**: Remove C++ dependencies
3. **Integrate**: Add to kernel_lib
4. **Boot Test**: Run in QEMU
5. **Profile**: Measure with rdtsc

---

## 📁 Project Structure

```
BareFlow-LLVM/
├── kernel_lib/           # Runtime library (15KB)
│   ├── io/              # VGA, Serial, Keyboard
│   ├── memory/          # malloc, string, compiler_rt
│   ├── cpu/             # rdtsc, cpuid, PIC, IDT
│   └── jit/             # Profiling system
├── tinyllama/           # Unikernel application (13KB)
├── tests/phase3/        # Phase 3 validation (17 programs)
├── docs/                # Documentation
│   └── phase3/          # Phase 3 results (5 documents)
├── archive/             # Old code (monolithic kernel)
├── boot/                # 2-stage bootloader
└── build/               # Build artifacts
```

---

## 🔧 Important Constraints

### Bare-Metal Limitations
- **No standard library**: All functions in kernel_lib/
- **No C++ exceptions**: Compile with `-fno-exceptions`
- **32-bit only**: All code with `-m32`
- **No floating point**: Use `-mno-sse -mno-mmx`

### Memory Layout
- **Bootloader**: 0x7C00 (Stage 1), 0x7E00 (Stage 2)
- **Kernel**: 0x10000 (64KB offset - safe from BIOS)
- **Stack**: 0x90000 (grows down)
- **Heap**: 0x100000 (256KB for malloc)

### Boot Process
1. **Stage 1** (512B): Load Stage 2 from sectors 1-8
2. **Stage 2** (4KB): Enable A20, setup GDT, load kernel
3. **Kernel**: Verify FLUD signature (0x464C5544)
4. **Entry**: Setup stack, call main()

---

## 🐛 Debugging

### Boot Issues
Error codes from Stage 2:
- `0xD1` = Disk read failure
- `0xA2` = A20 line cannot be enabled
- `0x51` = Invalid kernel signature
- `0xE0` = Kernel not found

Debug with: `make debug` in tinyllama/

### JIT Issues
1. Check LLVM version: `llvm-config-18 --version`
2. Verify linking: `ldd test_program`
3. Profile overhead: Use rdtsc timestamps
4. Memory usage: Monitor with custom allocator

---

## 📚 Key Documentation

### Architecture & Strategy
- `README.md` - Vision and "Grow to Shrink" philosophy
- `ROADMAP.md` - Current phase, tasks, timeline
- `ARCHITECTURE_UNIKERNEL.md` - Detailed unikernel design

### Phase 3 Results
- `docs/phase3/PHASE3_2_FINDINGS.md` - Static linking research
- `docs/phase3/PHASE3_3_RESULTS.md` - 399× speedup proof
- `docs/phase3/PHASE3_4_TIERED_JIT.md` - Tiered compilation
- `docs/phase3/PHASE3_5_DCE_RESULTS.md` - 99.83% dead code
- `docs/phase3/PHASE3_6_NATIVE_EXPORT.md` - 6000× reduction

### Implementation Details
- `BOOTLOADER_NOTES.md` - Boot process details
- `LLVM_PIPELINE.md` - 4-phase optimization strategy
- `JIT_ANALYSIS.md` - JIT implementation analysis

---

## ⚠️ Critical Rules

1. **Always profile before optimizing** - Use rdtsc for measurements
2. **Test in userspace first** - Validate concepts before bare-metal
3. **Keep binaries small** - Target <5MB for production
4. **Document metrics** - Include numbers in commits
5. **No external dependencies** - Everything self-contained

---

## 🎯 Success Criteria

### Phase 4 (Current)
- [ ] Custom LLVM ≤5MB
- [ ] Boot with LLVM ≤10s
- [ ] JIT works bare-metal
- [ ] 10× speedup after 100 boots

### Ultimate Goal
- [ ] 2-5MB final binary
- [ ] Hardware-optimal performance
- [ ] Zero manual optimization
- [ ] Persistent improvements

---

**Last Updated**: 2025-10-26 (Post-Session 22)
**Maintainer**: Claude Code Assistant
**Human**: @nickywan