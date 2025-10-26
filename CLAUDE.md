# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## ⚠️ CRITICAL: Project Philosophy - READ FIRST

### 🎯 "Grow to Shrink" - Size is NOT a Constraint!

**NEVER OPTIMIZE FOR INITIAL SIZE! THE SYSTEM MUST BE LARGE AT START!**

### ⚡ ARCHITECTURE CHANGE (Session 28): 64-bit Migration

**BareFlow is now x86-64 (64-bit) architecture!**

- ✅ All code compiled for x86-64 long mode
- ✅ LLVM native 64-bit (libLLVM-18.so)
- ✅ Better JIT performance (16 registers)
- ✅ No `-m32` flags anymore!

**Rationale**: See `docs/phase4/ARCH_DECISION_64BIT.md`

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

## 🔧 Runtime Strategy: Hybrid kernel_lib + llvm-libc

**BareFlow uses a HYBRID approach** for runtime functions:

### Custom (kernel_lib/) - Bare-Metal Drivers
**Keep custom implementations for:**
- **I/O**: VGA, serial, keyboard (hardware-specific, MMIO/ports)
- **CPU**: rdtsc, cpuid, PIC, IDT (x86-64 specific instructions)
- **Paging**: Page table setup (architecture-specific)

**Why custom**: These are bare-metal drivers with hardware side-effects, non-JIT-optimizable, already working.

### llvm-libc - JIT-Optimizable Functions
**Migrate to llvm-libc for:**
- **Memory**: malloc, free, realloc, calloc
- **String**: memcpy, memset, memmove, strlen, strcmp
- **Math** (future): sin, cos, sqrt, etc.

**Why llvm-libc**:
- Written in pure C (compilable to LLVM IR)
- JIT-optimizable (profiling + auto-vectorization)
- Example: `memcpy(dst, src, 512)` → JIT observes size → AVX2 specialized → **10× faster**

**Status**: Documented (Session 36), implementation planned for Phase 6 (Sessions 41-45)

**See**: `docs/architecture/LLVM_LIBC_STRATEGY.md` for complete migration plan

---

## 📌 Session Recovery Protocol

**IMPORTANT**: When starting a new session, follow this exact sequence:

### 1. Read Core Documents (in this order)
```bash
1. CLAUDE.md           # This file - project instructions
2. ROADMAP.md         # Current phase, next steps, timeline
3. README.md          # Vision and strategy overview
```

### 2. Read Important Reference Documents

**User Directive**: "ta mémoire doit aussi faire référence a des documents importants, si tu en crée un nouveau que tu estimes important mets en mémoire de le lire avant de poursuivre la session"

**Critical Reference Documents** (READ BEFORE CONTINUING):

1. **`docs/phase4/QEMU_BOOT_ISSUES_REFERENCE.md`** ⚠️ **CRITICAL**
   - All known QEMU boot problems and solutions
   - Created Session 30
   - **ALWAYS read before implementing new features**
   - Contains: malloc triple fault, BSS zeroing, flags, etc.

2. **`docs/phase4/SESSION_31_MALLOC_INVESTIGATION.md`** (500 lines)
   - Complete malloc investigation results (Session 31)
   - Problem isolated to free-list allocator
   - Bump allocator works (256 KB heap)
   - **READ before debugging malloc_llvm.c**

2b. **`docs/phase4/MALLOC_LLVM_DEBUG_SESSION32.md`** ⚠️ **CRITICAL** (700 lines)
   - Complete debug log of malloc_llvm.c (Session 32)
   - ROOT CAUSE: is_free not persisting (bool/size_t issue)
   - 7 systematic tests performed
   - **MUST READ before resuming malloc_llvm.c debug**
   - Status: DEFERRED (using bump allocator)

3. **`docs/phase5/PHASE5_PLANNING.md`** (850 lines)
   - Complete Phase 5 roadmap (Sessions 31-39)
   - Session 31: Paging implementation (CRITICAL)
   - **READ before starting any Phase 5 work**

4. **`docs/phase4/ARCH_DECISION_64BIT.md`**
   - Why 64-bit migration (not 32-bit)
   - Impact on all future work
   - **READ if questioning architecture choices**

**How to maintain this list**:
- When creating a NEW important document, add it here
- Mark with ⚠️ **CRITICAL** if blocking progress
- Include when to read it (before which session)
- Keep list under 10 documents (archive old ones)

### 3. Check Current State
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

### 4. Read Phase-Specific Documentation
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
4. **QEMU Validation**: ALWAYS test bare-metal code in QEMU
   - Use `qemu-system-x86_64` for 64-bit kernels
   - Create bootable ISO with GRUB/Multiboot2
   - Verify in real x86-64 environment
   - Serial output for debugging
5. **Profile**: Measure with rdtsc

**CRITICAL**: NEVER assume bare-metal code works without QEMU testing!

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
- **64-bit x86-64**: All code compiled for x86-64 (long mode)
- **No standard runtime**: Custom malloc_llvm, cpp_runtime

### Memory Layout (64-bit)
- **Bootloader**: 0x7C00 (Stage 1), 0x7E00 (Stage 2) OR Multiboot2/GRUB
- **Kernel**: Higher-half (0xFFFFFFFF80000000+) OR identity mapped
- **Stack**: 8MB (64-bit requires larger stack)
- **Heap**: 200MB (malloc_llvm for LLVM + TinyLlama)

### Boot Process (64-bit)
**Option A - Multiboot2/GRUB** (Recommended):
1. **GRUB**: Loads kernel in 64-bit long mode
2. **Kernel**: Entry point receives Multiboot2 info
3. **Init**: Setup stack, heap, serial I/O
4. **Main**: Call main()

**Option B - Custom Bootloader**:
1. **Stage 1** (512B): Load Stage 2
2. **Stage 2**: Enable A20, setup page tables, enter long mode
3. **Kernel**: Load at higher-half or identity mapped
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

### Phase 4 (Complete ✅)
- [x] 64-bit runtime library (kernel_lib_llvm.a 29 KB)
- [x] QEMU bare-metal validation infrastructure
- [x] Tiered compilation validated (1.7× speedup O0→O1)
- [x] LLVM integration proven (userspace + bare-metal)
- [x] malloc investigation documented (deferred to Phase 5)

### Phase 5 (Current)
- [ ] Paging implementation (4-level page tables)
- [ ] malloc working with paging
- [ ] TinyLlama model loading (~60MB)
- [ ] Inference optimization (matrix multiply, vectorization)
- [ ] JIT hot layer compilation

### Ultimate Goal
- [ ] 2-5MB final binary
- [ ] Hardware-optimal performance
- [ ] Zero manual optimization
- [ ] Persistent improvements

---

## 📌 CRITICAL: Session Guidelines (User Directives)

### 1. Update README.md Every Session

**User Request**: "garde en mémoire de mettre à jour aussi le README principal si nécessaire à chaque session"

**When to update README.md**:
1. **Major phase completion** (Phase 3 → Phase 4 → Phase 5)
2. **Architecture decisions** (32-bit → 64-bit migration)
3. **Significant milestones** (QEMU boot working, malloc investigation)
4. **Session completion** (update current status, results)

**How to update**:
- Update Phase status (EN COURS → COMPLÈTE)
- Add session results (Session X ✅: brief description)
- Update "Next Phase" section if transitioning
- Update metrics if significant changes

### 2. Test in QEMU, Not Userspace

**User Request**: "garde ne mémoire de tester en userspace que si c'est strictement necessaire sinon qemu"

**Testing Policy**:
- ✅ **DEFAULT**: Test in QEMU (qemu-system-x86_64)
- ✅ **ALWAYS**: Bare-metal code MUST be tested in QEMU
- ❌ **AVOID**: Userspace testing unless strictly necessary
- ✅ **EXCEPTION**: Userspace OK for rapid prototyping if it saves significant time

**Rationale**:
- QEMU provides real x86-64 environment
- Catches issues userspace can't (paging, interrupts, BSS, etc.)
- Faster feedback than hardware
- Reproducible test environment

**When userspace IS acceptable**:
- Quick LLVM API prototyping (30 min vs 2 hours in QEMU)
- Testing C++ standard library compatibility
- Benchmarking algorithms (not memory/boot behavior)

**When userspace is NOT acceptable**:
- Testing malloc (needs real memory layout)
- Testing boot sequence (needs Multiboot2/GRUB)
- Testing interrupts/exceptions
- Testing memory paging
- **Final validation**: ALWAYS in QEMU

### 3. Reference Boot Issues from Docs and Commits

**User Request**: "garde aussi en mémoire tous les problèmes de boot et leur résolution sur qemu, c'est dans les *.md ou sinon dans les message commits"

**Boot Issue Knowledge Base**:
- **Primary Reference**: `docs/phase4/QEMU_BOOT_ISSUES_REFERENCE.md` (created Session 30)
- **Investigation Reports**: `docs/phase4/MALLOC_INVESTIGATION.md`
- **Commit Messages**: Search with `git log --grep="fix\|Fix\|error\|Error"`

**Known Issues (memorized)**:
1. **malloc triple fault** → Needs paging (Session 31)
2. **Entry point naming** → Use `main()` not `kernel_main()`
3. **Serial function** → Use `serial_putchar()` not `serial_putc()`
4. **BSS not zeroed** → Manual `rep stosb` in boot.S
5. **Assembly syntax** → Use `movabs` for 64-bit addresses
6. **Red zone corruption** → Use `-mno-red-zone` flag
7. **Code model** → Use `-mcmodel=kernel` flag

**Before implementing new features**:
1. Check `QEMU_BOOT_ISSUES_REFERENCE.md` for known issues
2. Review recent commit messages for similar problems
3. Apply known solutions proactively

---

## 🎓 Lessons Learned - Phase 4

### 1. QEMU Validation is Essential
**Lesson**: ALWAYS test bare-metal code in QEMU before assuming it works.
- Real x86-64 environment catches issues userspace can't
- Serial debugging is invaluable (COM1 port)
- Faster iteration than hardware testing
- Create reproducible test environment (Multiboot2 + GRUB + ISO)

### 2. Architecture Decisions Have Long-Term Impact
**Lesson**: 64-bit migration simplified development significantly.
- Native LLVM support (no 3+ hour custom builds)
- Better JIT performance (16 registers vs 8)
- Modern toolchain compatibility
- Initial complexity pays off long-term

**Flags to remember (64-bit kernel)**:
```
-mno-red-zone       # CRITICAL for 64-bit kernel (no stack red zone)
-mcmodel=kernel     # Higher-half addressing model
-fno-pie            # No position-independent executable
-fno-stack-protector # No stack canary checks
```

### 3. Commit at Each Step
**User Request**: "n'oublie pas de commiter à chaque step mets ca en mémoire"

**Practice**:
- Commit after each significant change
- Write detailed commit messages with context
- Include session number and result
- Never batch multiple unrelated changes

**Example commit message format**:
```
feat(phase4): Session X - Brief description

Detailed explanation of what was done, why, and results.

Results:
- Metric 1: Value
- Metric 2: Value

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

### 4. Investigate Thoroughly, Defer Pragmatically
**Lesson**: malloc investigation wasn't wasted time.
- 7 different debugging approaches tried
- Clear findings enable Phase 5 work
- Documented hypothesis provides roadmap
- Deferred decisions need strong rationale

**When to defer**:
- Extensive investigation done (not first try)
- Root cause hypothesis documented
- Not blocking current phase completion
- Clear path to resolution in future phase

### 5. Documentation is Development
**Lesson**: 2500+ lines of documentation in Phase 4 is not overhead.
- Enables session recovery after context loss
- Provides decision record (why, not just what)
- Investigation reports save future debugging time
- Architecture decision records prevent revisiting settled questions

**Documentation types**:
- Session summaries (what happened, results, next steps)
- Investigation reports (malloc, performance, etc.)
- Architecture decision records (ADR format)
- Technical references (API docs, build guides)

### 6. Userspace Validation First
**Lesson**: test_tiered_llvm.cpp validated entire approach before bare-metal.
- Faster iteration cycle (compile, run, debug)
- Easier debugging (gdb, printf, valgrind)
- Proves concept before complexity
- Metrics become baseline for bare-metal comparison

**Process**:
1. Prototype in userspace (C++)
2. Validate metrics and behavior
3. Port to C (remove C++ dependencies)
4. Integrate into kernel_lib
5. Test in QEMU bare-metal
6. Compare metrics to userspace baseline

---

**Last Updated**: 2025-10-26 (Post-Session 30 - Phase 4 COMPLETE)
**Maintainer**: Claude Code Assistant
**Human**: @nickywan