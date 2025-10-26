# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 📌 Quick Context Recovery

**For continuing a previous session**, read `CLAUDE_CONTEXT.md` first - it contains:
- Current project state and recent changes
- Last session's work and commits
- Known issues and solutions
- Exact next steps to take

## Project Overview

**BareFlow** is a self-optimizing unikernel implementing the "Grow to Shrink" strategy - a hybrid AOT+JIT system that starts large (60MB with full LLVM), profiles itself, optimizes hot paths, eliminates dead code, and converges to a minimal native binary (2-5MB).

**Current State**: Phase 3.6 complete in userspace - all validation done, ready for bare-metal integration.

**Architecture**: Unikernel design (28KB) with kernel_lib.a runtime (15KB) + application (13KB). No syscalls, direct function calls, Ring 0 execution.

## Build Commands

### Bare-Metal Kernel
```bash
# Build the complete bootable kernel
make                  # Build everything (stage1, stage2, kernel)
make run              # Build and launch in QEMU with serial output
make debug            # Build and run with CPU debug logs
make clean            # Remove all build artifacts
make rebuild          # Clean + build
```

### JIT Interface (LLVM 18)
```bash
# Requires: llvm-18-dev, clang-18, llvm-18-tools
make -f Makefile.jit info           # Show LLVM configuration
make -f Makefile.jit all            # Build JIT tests and minimal.bc library
make -f Makefile.jit libs           # Build only minimal.bc (LLVM bitcode library)
make -f Makefile.jit test-basic     # Test basic LLVM LLJIT
make -f Makefile.jit test-interface # Test JIT interface with profiling
make -f Makefile.jit clean          # Clean JIT build artifacts
```

### Module System (AOT Compiled Modules)
```bash
make -f Makefile.modules all          # Compile all modules in modules/ to .mod files
make -f Makefile.modules list-modules # List compiled modules with sizes
make -f Makefile.modules clean        # Clean module build artifacts
```

### Phase 3 JIT Testing (Userspace Validation)
```bash
# Basic JIT test
make -f Makefile.jit test-basic

# LLVM Interpreter vs JIT comparison (Phase 3.3)
clang++-18 -g test_llvm_interpreter.cpp -o test_llvm_interpreter \
  $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native)
./test_llvm_interpreter

# Tiered JIT compilation (Phase 3.4)
clang++-18 -g test_tiered_jit.cpp -o test_tiered_jit \
  $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native)
./test_tiered_jit

# Matrix multiply optimization benchmark
clang++-18 -g test_matmul_O0.cpp -o test_matmul_O0 \
  $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native)
./test_matmul_O0

# Native code export (Phase 3.6)
clang++-18 -g test_native_export.cpp -o test_native_export \
  $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native)
./test_native_export

# Dead code analysis
./analyze_llvm_usage.sh
```

### Unikernel Testing
```bash
cd tinyllama
make run              # Build and boot in QEMU with serial output
make debug            # Boot with CPU debug logs
```

## Architecture

### Boot Process
The system uses a **two-stage bootloader** to transition from BIOS to 32-bit protected mode:

1. **Stage 1** (`boot/stage1.asm`): 512-byte MBR bootloader loaded by BIOS at 0x7C00
   - Loads Stage 2 from sectors 1-8 to 0x7E00
   - Minimal functionality, CHS disk access only

2. **Stage 2** (`boot/stage2.asm`): 4KB extended bootloader at 0x7E00
   - Enables A20 line (BIOS, keyboard controller, or Fast A20 methods)
   - Loads kernel from sector 9+ to 0x1000 using LBA (with CHS fallback)
   - Verifies kernel signature ("FLUD" magic bytes: 0x464C5544)
   - Sets up GDT with flat 4GB segments (code at 0x08, data at 0x10)
   - Transitions to protected mode and jumps to kernel at 0x1000

3. **Kernel Entry** (`kernel/entry.asm`): Assembly entry point
   - Must start with "FLUD" signature (validated by Stage 2)
   - Sets up initial stack at 0x90000
   - Calls `kernel_main()` in C

### Disk Layout
```
Sector 0:       Stage 1 bootloader (512 bytes)
Sectors 1-8:    Stage 2 bootloader (4096 bytes)
Sectors 9+:     Kernel binary (variable size)
```

### "Grow to Shrink" Architecture

BareFlow implements a **progressive convergence** strategy inspired by PyPy, LuaJIT, and 68000-style self-tuning programs:

```
Boot 1-10:   [60-118MB] Full LLVM + app in IR → Interpreted (slow but profiles everything)
Boot 10-100: [30-60MB]  Hot paths JIT O0→O3   → 10× faster
Boot 100+:   [2-5MB]    Native export          → LLVM removed, pure AOT code
```

#### Phase 1-2: AOT Baseline (COMPLETE ✅)
- **Unikernel**: 28KB (13KB app + 15KB kernel_lib.a)
- **Profiling**: Cycle-accurate rdtsc measurement
- **Boot**: 2-stage bootloader, protected mode, VGA/Serial I/O
- **Performance**: 24 cycles/call overhead, direct function calls

#### Phase 3: Hybrid Self-Optimizing Runtime (COMPLETE ✅ in userspace)

**Phase 3.1**: LLVM JIT Verification
- ✅ Validated LLVM 18.1.8 OrcJIT works
- ✅ Measured sizes: 31KB binary + 118MB libLLVM-18.so (dynamic)
- ✅ Decision: Use dynamic linking for development, custom build for production

**Phase 3.2**: Static Linking Research
- ✅ Tested: 27MB minimal static (invalid), 110MB full static (invalid)
- ✅ Root cause: Ubuntu LLVM built for dynamic linking only
- ✅ Strategy: Hybrid approach - dynamic for dev, MinSizeRel custom build later

**Phase 3.3**: Interpreter vs JIT Validation
- ✅ AOT (clang -O2): 0.028ms baseline
- ✅ Interpreter: 13.9ms (498× slower - universal profiling mode)
- ✅ JIT: 0.035ms (1.25× slower - near-optimal)
- ✅ **Speedup: 399× from Interpreter to JIT!**

**Phase 3.4**: Tiered Compilation
- ✅ Automatic recompilation: O0 (0-99 calls) → O1 (100-999) → O2 (1000-9999) → O3 (10000+)
- ✅ 50,000 iterations tested on fibonacci(30)
- ✅ Compilation overhead: 0.007% of total time (negligible)
- ✅ JIT O3 performance: 1.17× slower than AOT (acceptable)

**Phase 3.5**: Dead Code Analysis
- ✅ Total LLVM symbols: 32,451
- ✅ Used by test: 54 (0.17%)
- ✅ **Dead code: 99.83%!**
- ✅ Current size: 118MB → Potential: 2-5MB (95-98% reduction)

**Phase 3.6**: Native Export
- ✅ JIT runtime: 118MB (development)
- ✅ Native snapshot: 20KB (production - 3 hot functions + runtime)
- ✅ **Size reduction: 99.98% (6000× smaller!)**
- ✅ Performance: Same as JIT O3 (4.04ms)
- ✅ **Strategy validated end-to-end!**

#### Next: Bare-Metal Integration (Phase 4)
- Port LLVM OrcJIT to bare-metal (custom allocator, no exceptions)
- Build minimal LLVM (X86 only, MinSizeRel, 2-5MB)
- Implement persistence to FAT16 filesystem
- Integrate TinyLlama model for real workload

### Key Components

#### Runtime Library (`kernel_lib/` → `kernel_lib.a` 15KB)
- **I/O**: VGA 80×25 text mode (0xB8000), Serial COM1 115200, PS/2 Keyboard
- **Memory**: Bump allocator (256KB heap), memcpy/memset/strlen, compiler_rt (__udivdi3, __divdi3)
- **CPU**: rdtsc profiling, cpuid features, PIC (8259), IDT (interrupt handling)
- **JIT**: Per-function profiling (call count, min/max/avg cycles)
- **API**: `runtime.h` (I/O + Memory + CPU), `jit_runtime.h` (profiling)

#### TinyLlama Unikernel (`tinyllama/` → `tinyllama_bare.bin` 13KB)
- **Entry**: `entry.asm` with FLUD signature (0x464C5544), stack at 0x90000
- **Main**: Self-profiling demo (fibonacci, sum_to_n, count_primes)
- **Profiling**: jit_profile_begin/end wrappers around hot functions
- **Output**: VGA + Serial with detailed timing statistics
- **Build**: Single binary (28KB total), no syscalls, Ring 0

#### Linker Script (`tinyllama/linker.ld`)
- Places code at 0x10000 physical address (64KB - safe from BIOS)
- Section order: .text, .rodata, .data, .bss
- Aligns sections to 4096-byte boundaries

#### Bootloader (`boot/stage1.asm` + `boot/stage2.asm`)
- **Stage 1**: 512-byte MBR, loads Stage 2 from sectors 1-8
- **Stage 2**: 4KB loader, A20 enable, GDT setup, protected mode transition
- **Load**: Reads 64 sectors (32KB) from sector 9 to 0x10000 (LBA + CHS fallback)
- **Verify**: Checks FLUD signature before jumping to kernel

## Development Workflow

### Modifying the Unikernel Application
1. Edit `tinyllama/main.c` for application logic
2. Add profiling with `jit_profile_begin("func")` / `jit_profile_end("func")`
3. Build: `cd tinyllama && make`
4. Test: `make run` (boots in QEMU with serial output)
5. Debug: `make debug` (enables CPU debug logs)

### Adding Runtime Library Features
1. Add implementation to `kernel_lib/{io,memory,cpu,jit}/`
2. Export function in `kernel_lib/runtime.h` or `kernel_lib/jit_runtime.h`
3. Update `kernel_lib/Makefile` to include new .c files
4. Rebuild: `cd kernel_lib && make rebuild`
5. Relink application: `cd tinyllama && make rebuild`

### Testing JIT Features (Userspace)
1. Create test in root directory (e.g., `test_my_feature.cpp`)
2. Compile with: `clang++-18 -g test_my_feature.cpp -o test_my_feature $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native)`
3. Run: `./test_my_feature`
4. Validate results before bare-metal integration

### Adding Phase 3 Features to Bare-Metal
1. **Validate in userspace first** (create test_*.cpp program)
2. **Port to C**: Remove C++ stdlib dependencies, use kernel_lib APIs
3. **Add to kernel_lib**: Place in appropriate subdirectory
4. **Test incrementally**: Build small, test often
5. **Measure overhead**: Use rdtsc profiling to track performance impact

### Debugging Boot Issues
- Error codes displayed by Stage 2:
  - `0xD1` = Disk read failure
  - `0xD2` = Incorrect sector count
  - `0xA2` = A20 line cannot be enabled
  - `0x51` = Invalid kernel signature (expected "FLUD")
  - `0xE0` = Kernel not found
- Use `make debug` to see CPU state and interrupt logs
- Verify kernel signature: `hexdump -n 4 -e '4/1 "%02x"' build/kernel.bin` should show `44554c46`

## Important Constraints

### Bare-Metal Limitations
- **No standard library**: All C runtime functions must be implemented in `kernel/stdlib.c`
- **No dynamic allocator** for C++: JIT requires custom memory management
- **No exceptions**: Compile with `-fno-exceptions` for bare-metal C++
- **32-bit only**: All code compiled with `-m32`
- **No floating point**: Use `-mno-sse -mno-mmx` if adding FP later

### Bootloader Constraints
- Stage 1 must be exactly 512 bytes (enforced by Makefile)
- Stage 2 must be exactly 4096 bytes (8 sectors)
- Kernel must start with "FLUD" signature (validated before execution)
- Stack grows down from 0x7C00 in Stage 1, relocated to 0x90000 in kernel

### Module System
- AOT modules must be compiled with `-m32 -ffreestanding -nostdlib`
- Module functions must match signature: `int function_name(void)`
- Maximum 16 modules (MAX_MODULES in module_loader.h)
- Module names limited to 32 characters

## Code Organization

```
BareFlow-LLVM/
├── boot/                          # Bootloader (2-stage)
│   ├── stage1.asm                 # MBR (512 bytes)
│   └── stage2.asm                 # Extended loader (4KB)
│
├── kernel_lib/                    # Runtime library (→ kernel_lib.a 15KB)
│   ├── io/
│   │   ├── vga.{h,c}              # VGA 80×25 text mode
│   │   ├── serial.{h,c}           # COM1 serial port
│   │   └── keyboard.h             # PS/2 keyboard input
│   ├── memory/
│   │   ├── malloc.{h,c}           # Bump allocator (256KB heap)
│   │   ├── string.{h,c}           # memcpy, memset, strlen, etc.
│   │   └── compiler_rt.c          # __udivdi3, __divdi3 (64-bit division)
│   ├── cpu/
│   │   ├── features.h             # rdtsc, cpuid
│   │   ├── pic.{h,c}              # 8259 PIC interrupt controller
│   │   └── idt.{h,c}              # Interrupt descriptor table
│   ├── jit/
│   │   └── profile.{h,c}          # Function profiling system
│   ├── runtime.h                  # Public API: I/O + Memory + CPU
│   ├── jit_runtime.h              # Public API: JIT profiling
│   └── Makefile                   # Build kernel_lib.a
│
├── tinyllama/                     # Unikernel application (→ 13KB)
│   ├── entry.asm                  # Entry point with FLUD signature
│   ├── main.c                     # Self-profiling demo
│   ├── linker.ld                  # Linker script (code at 0x10000)
│   ├── Makefile                   # Build system
│   ├── tinyllama_bare.elf         # ELF binary (18KB, generated)
│   ├── tinyllama_bare.bin         # Raw binary (13KB, generated)
│   └── tinyllama.img              # Bootable disk image (10MB, generated)
│
├── Phase 3 Tests (userspace validation)
│   ├── test_llvm_interpreter.cpp  # Phase 3.3: Interpreter vs JIT
│   ├── test_tiered_jit.cpp        # Phase 3.4: O0→O1→O2→O3
│   ├── test_matmul_O*.cpp         # Quick win: Matrix multiply
│   ├── test_native_export.cpp     # Phase 3.6: Native code export
│   └── analyze_llvm_usage.sh      # Phase 3.5: Dead code analysis
│
├── build/                         # Build artifacts
│   ├── stage1.bin                 # Bootloader stage 1
│   ├── stage2.bin                 # Bootloader stage 2
│   └── *.o                        # Object files
│
├── docs/                          # Documentation
│   ├── PHASE3_*.md                # Phase 3 results and findings
│   └── archive/                   # Old architecture docs
│
└── Old/Archived
    ├── kernel/                    # Old monolithic kernel (archived)
    ├── modules/                   # Old module system (archived)
    └── libs/minimal/              # LLVM bitcode lib (for JIT tests)
```

## Performance Notes

### AOT Baseline (Phase 1-2)
Measured on bare-metal unikernel (tinyllama/main.c):
- **Binary size**: 28KB (13KB app + 15KB runtime)
- **Function call overhead**: 24 cycles/call (direct, inline)
- **Boot time**: ~3.1M cycles initialization
- **Total execution**: ~16.4M cycles (3 test functions)

Per-function profiling (AOT-compiled, -O2):
- `fibonacci(10)`: avg=32,638 cycles, min=6,235, max=267,864 (43× variation)
- `sum_to_n(1000)`: avg=549 cycles, min=174, max=19,960 (115× variation)
- `count_primes(100)`: avg=50,451 cycles, min=10,088, max=209,608 (21× variation)

Variation due to cache effects, branch mispredictions.

### JIT Performance (Phase 3 - Userspace)

**Interpreter vs JIT** (fibonacci(20), 10 iterations):
- AOT (clang -O2): 0.028ms (baseline)
- Interpreter: 13.9ms (498× slower - universal profiling)
- JIT O3: 0.035ms (1.25× slower - near-optimal)
- **Speedup: 399× from Interpreter to JIT!**

**Tiered Compilation** (fibonacci(30), 50,000 iterations):
- AOT (clang -O2): 3.43ms (baseline)
- JIT O0 (0-99 calls): 4.20ms
- JIT O1 (100-999): 4.02ms
- JIT O2 (1000-9999): 4.03ms
- JIT O3 (10000+): 4.04ms
- Compilation overhead: 0.007% (negligible)

**Matrix Multiply** (512×512):
- O0 (no opt): 5.83ms
- O2 (aggressive): 1.70ms (3.42× faster)
- O3 (maximum): 1.82ms (3.20× faster)

**Native Export**:
- JIT runtime: 118MB (dev phase)
- Native snapshot: 20KB (3 functions + runtime)
- Size reduction: 99.98% (6000× smaller)
- Performance: Same as JIT O3

## Related Documentation

**Current Architecture**:
- `CLAUDE_CONTEXT.md`: Session history, current state, exact next steps
- `README.md`: High-level vision, "Grow to Shrink" strategy
- `ARCHITECTURE_UNIKERNEL.md`: Detailed unikernel architecture

**Phase 3 Results** (userspace validation):
- `PHASE3_2_FINDINGS.md`: Static linking research
- `PHASE3_3_RESULTS.md`: Interpreter vs JIT validation (399× speedup)
- `PHASE3_4_TIERED_JIT.md`: Tiered compilation (O0→O3)
- `PHASE3_5_DCE_RESULTS.md`: Dead code analysis (99.83% unused)
- `docs/PHASE3_6_NATIVE_EXPORT.md`: Native export (6000× reduction)

**Historical/Archived**:
- `docs/archive/`: Old monolithic kernel documentation
- `BAREFLOW_MODULE_SYSTEM.md`: Old module system (archived)
- `JIT_MERGE_README.md`: Old JIT interface docs
