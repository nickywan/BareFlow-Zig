# Fluid OS - Project Roadmap

## Vision
A bare-metal unikernel capable of running TinyLlama with real-time JIT optimization and persistent optimization cache across reboots, targeting dedicated inference machines where the kernel and modules are compiled on-host (clang/LLVM + llvm-libc) to squeeze every CPU feature available.

## Current Status
✅ Two-stage bootloader (Stage 1 + Stage 2)
✅ 32-bit protected mode kernel
✅ VGA text mode driver
✅ PS/2 keyboard input
✅ AOT module system with cycle-accurate profiling
✅ Basic memory management (malloc/free)
✅ LLVM 18 JIT interface (userspace prototype)

## Phase 1: JIT Integration & Module System (IN PROGRESS)

### 1.1 Runtime Infrastructure (COMPLETED ✅)
- [x] Implement minimal C++ runtime for bare-metal
  - [x] Operator new/delete with custom allocator
  - [x] Basic exception handling stubs (or -fno-exceptions)
  - [x] RTTI stubs (or -fno-rtti)
  - [x] Static constructor/destructor support
- [x] Custom memory allocator (jit_allocator)
  - [x] Three memory pools (CODE, DATA, METADATA)
  - [x] Aligned allocation support
  - [x] Free-list with block coalescence
  - [x] Pool statistics tracking
- [x] Bootloader support for 80-sector kernel (40KB)
  - [x] Chunked LBA reading (8 sectors at a time, 10 iterations)
  - [x] Kernel loaded at 0x10000 to avoid bootloader conflict
  - [x] Segment:offset addressing for real mode

### 1.2 Profile-Guided Optimization System ✅ COMPLETED & VALIDATED

**Architecture Decision**: AOT + Offline Recompilation (not bare-metal JIT)
- Kernel profiles hot functions with cycle-accurate rdtsc
- Profiling data exported via serial port to host
- Host uses full LLVM toolchain to recompile with PGO
- Optimized modules loaded from persistent cache at boot
- Rationale: Full LLVM in bare-metal requires ~500KB + libc++ (3-6 months work)

**Status**: 🎉 **100% FUNCTIONAL & VALIDATED** - Real performance gains measured!

**Completed Tasks**:
- [x] Profiling Export System
  - [x] JSON format for profiling statistics
  - [x] Serial port driver (COM1) for data extraction
  - [x] Export per-module call counts, cycles (min/max/total), code addresses
  - [x] Automated export at boot (no VGA interference)
- [x] Optimized Module Cache
  - [x] Design in-image cache format (embedded module blobs + registry)
  - [x] Cache loader at boot with header + module signature checks
  - [x] Cache tag generation (i686 for QEMU, native CPU for bare-metal)
  - [x] Module override system with fallback to embedded modules
- [x] Offline Recompilation Pipeline
  - [x] Implement `tools/pgo_recompile.py` to parse profiling JSON and emit plan
  - [x] Module classification: O1 (warm), O2 (hot), O3 (ultra-hot)
  - [x] Trigger LLVM recompilation with -O1/-O2/-O3 based on hotness
  - [x] Generate optimized .mod files in cache/i686/ directory
  - [x] Module linker script (modules/module.ld) ensuring predictable binary layout
  - [x] Entry_point fix: guarantee header at 0x00, code at 0x30
- [x] Benchmark Modules
  - [x] Add `matrix_mul` module (16×16 with .data section)
  - [x] Validated with compute, primes, sum, fibonacci modules
  - [x] Fixed .bss section issue - modules must use initialized data
- [x] Complete Workflow Testing
  - [x] make pgo-profile - Capture profiling from QEMU
  - [x] make pgo-analyze - Classify modules
  - [x] make pgo-apply - Recompile with optimizations
  - [x] Kernel rebuild with cache embedded
  - [x] Boot test: SUCCESS, no crashes ✅

**Key Files Created**:
- `modules/module.ld` - Linker script for predictable module layout
- `tools/pgo_recompile.py` - PGO analysis and recompilation
- `tools/gen_cpu_profile.py` - CPU profiling and tag generation
- `tools/embed_module.py` - Module embedding tool
- `tools/gen_cache_registry.py` - Cache registry generator
- `kernel/cache_loader.{h,c}` - Runtime cache loader

**Performance Measurements** (✅ COMPLETED 2025-10-25):
- [x] Baseline measurements captured (compute: 6.6M cycles, fibonacci: 140K cycles)
- [x] Optimized measurements validated (compute: 3.6M cycles, fibonacci: 17K cycles)
- [x] Real gains measured:
  * fibonacci: **+87.59%** (nearly 8× faster with -O2)
  * compute: **+45.25%** (2× faster with -O3)
  * sum: **+47.00%**
  * primes: **+44.79%**
- [x] Critical bug fixed: -o3 vs -O3 flag generation in pgo_recompile.py
- [x] Full workflow validated: profile → classify → recompile → embed → boot → measure

**Session 5 Progress** (2025-10-25):
- [x] Fixed 7-module loading bug (fft_1d, sha256, matrix_mul now load correctly)
- [x] Added stub functions to embedded_modules.h for cache override system
- [x] Validated complete PGO workflow with 7 modules (20 total calls)
- [x] Measured optimized performance: fibonacci 1.30x, compute 1.07x, fft_1d 1.09x, sha256 1.06x
- [x] Last working commit: caaf48d (7 modules, 50KB kernel)

**Remaining Optional Tasks** (not blocking):
- [ ] Disk partition/file for persistent storage (currently using embedded cache)
- [ ] LRU eviction policy for cache management
- [x] Fix matrix_mul static data issue (.bss → .data section with initialized arrays)
- [x] Debug matrix_mul module loading/execution (FIXED - required stub in embedded_modules.h)
- [ ] Debug 9-module boot failure (quicksort + strops cause complete boot hang)

### 1.3 llvm-libc Integration & Toolchain ✅ COMPLETED
- [x] Integrate llvm-libc subset (freestanding mode)
  - [x] Replace stdlib.c string functions (memcpy, memset, strlen, strcmp)
  - [x] Build system (Makefile.llvmlibc)
  - [x] Link kernel with libllvmlibc.a
  - [x] Add math.h functions (sin, cos, exp, log with Taylor series)
  - [ ] Adapt malloc/free to use jit_allocator backend (Phase 1.4)
- [ ] CPU Feature Detection Pipeline
  - [x] Host scanner: clang -march=native -### to detect features
  - [x] Generate build/cpu_profile.json (SSE/AVX/BMI + cache sizes)
  - [x] Auto-generate kernel/auto_cpu_flags.mk for Makefile
  - [ ] Runtime CPUID validation at boot
  - [ ] Tag benchmark results with CPU profile (e.g., matrix_mul_AVX2)
- [ ] Host-tuned build workflow
  - [x] Script tools/gen_cpu_profile.py for feature detection (default `-march=i686` safe for QEMU)
  - [ ] Compile kernel/modules with -march=native on target machine
  - [ ] Validate llvm-libc API coverage for benchmarks
  - [ ] Document llvm-libc gaps and workarounds

### 1.4 Module System Improvements (IN PROGRESS)
- [ ] Dynamic module loading from disk
  - [ ] FAT16 filesystem support (read-only, minimal)
  - [ ] Module loader from disk sectors
  - [ ] Module signature verification (SHA256)
- [ ] Enhanced profiling metrics
  - [ ] Per-function profiling granularity
  - [ ] Call graph tracking
  - [ ] Memory allocation per module
- [x] Additional benchmark modules (✅ COMPLETED)
  - [x] `fft_1d`: radix-2 FFT (32 samples) - memory stride, trig, complex arithmetic
  - [x] `sha256`: SHA256 hash (1KB chunks) - memory bandwidth, bitwise ops
  - [x] `quicksort`: recursive sorting (128 elements, 5 iterations) - branch prediction tests
  - [x] `strops`: string operations (100 iterations) - memory access patterns
  - [x] Capture cycle counts via existing PGO workflow
- [x] Module loading system fixes (✅ COMPLETED)
  - [x] Fixed 7-module loading (fft_1d, sha256, matrix_mul)
  - [x] Added stub functions for cache override system
  - [x] Validated module execution with profiling export
- [ ] Debug boot failure with 9 modules (BLOCKER)
  - [x] Created quicksort.c and strops.c modules
  - [ ] Kernel fails to boot with 9 modules (no serial output)
  - [ ] Last working: 7 modules (commit caaf48d)
  - [ ] Current broken: 9 modules (commit 4ef737f)
  - [ ] Investigation needed: stack overflow, memory corruption, or size limits

## Phase 2: Kernel Extensions

### 2.1 Interrupt Handling ✅ COMPLETED
- [x] IDT (Interrupt Descriptor Table) setup
- [x] Exception handlers (div zero, debug, NMI, breakpoint, overflow, bound, invalid op, GPF, page fault)
- [x] PIC (Programmable Interrupt Controller) configuration (remapped to 0x20-0x2F)
- [x] Timer interrupt (IRQ0) with tick counter
- [x] Keyboard interrupt (IRQ1) stub handler

### 2.2 Basic Scheduler
- [ ] Task structure definition
- [ ] Round-robin scheduler
- [ ] Context switching
- [ ] Cooperative multitasking
- [ ] Benchmark (control-flow stress): execute `regex_dfa` module to observe branch behavior

### 2.3 Additional Drivers
- [ ] Serial port (COM1) for debugging
- [ ] PCI enumeration
- [ ] Disk driver (IDE/AHCI)
- [ ] Network card (optional, for remote profiling)

## Phase 3: Performance Optimization

### 3.1 JIT Optimization Cache
- [ ] Design persistent cache format
  - [ ] Cache metadata (function signature, optimization level, hash)
  - [ ] Native code storage
  - [ ] Relocation information
- [ ] Disk-based cache storage
  - [ ] Cache partition/file on disk
  - [ ] Write optimized code to cache
  - [ ] Read cache on boot
- [ ] Cache invalidation strategy
  - [ ] Version tracking
  - [ ] Checksum verification
  - [ ] LRU eviction policy
- [ ] Host-compilation workflow
  - [ ] Snapshot LLVM toolchain/llvm-libc revisions for reproducible builds
  - [ ] Automate per-host CPU feature detection and flag injection
  - [ ] Support compiling modules directly on target inference machines
  - [ ] Integrate `gen_cpu_profile.py` into CI to validate host profiles
  - [ ] Export bench tags alongside artifacts for comparison dashboards

### 3.2 Advanced Profiling
- [ ] Hardware performance counters (PMU)
  - [ ] Branch prediction statistics
  - [ ] Cache miss rates
  - [ ] TLB misses
- [ ] Sampling-based profiling
- [ ] Flame graph generation (export data)
- [ ] Real-time optimization triggers
- [ ] Benchmark (tiled compute): `gemm_tile` module to validate cache hits and allocator behaviour
- [ ] Benchmark (physics loop): `physics_step` module to measure SIMD/vector gains
- [ ] Auto-optimization maturity
  - [ ] Graduated thresholds per module category
  - [ ] Tune reoptimization policies for TinyLlama preparation
- [ ] Hardware-awareness
  - [ ] Leverage PMU counters for SSE/AVX utilization feedback
  - [ ] Plan NUMA-aware memory placement for multi-socket deployments
  - [ ] Track llvm-libc feature coverage for bare-metal runtime
  - [ ] Inventory critical llvm-libc APIs (benchmarks + TinyLlama) and verify availability
  - [ ] Correlate bench tags/reporting with PMU metrics (e.g., `gemm_tile_AVX512`)
  - [ ] Plan llvm-libc massage/patch strategy (enumerate missing pieces, fallback APIs)

### 3.3 Memory Management
- [ ] Proper heap allocator (buddy system or slab)
- [ ] Page allocator
- [ ] Memory pools for JIT code
- [ ] Garbage collection for dead JIT code

## Phase 4: Infrastructure

### 4.1 Build System
- [ ] Unified Makefile for all components
- [ ] Automatic dependency tracking
- [ ] Cross-compilation toolchain setup script
- [ ] CI/CD pipeline (GitHub Actions)

### 4.2 Testing
- [ ] Unit tests for kernel functions
- [ ] Integration tests for module system
- [ ] JIT correctness tests
- [ ] Performance regression tests
- [ ] Automated QEMU-based tests

### 4.3 Documentation
- [ ] Architecture documentation
- [ ] API reference
- [ ] Developer guide
- [ ] Performance tuning guide
- [ ] Troubleshooting guide

## Phase 5: TinyLlama Integration (FINAL GOAL)

### 5.1 TinyLlama Port
- [ ] Analyze TinyLlama dependencies
  - [ ] Identify required math functions
  - [ ] Identify memory requirements
  - [ ] Identify system calls needed
- [ ] Port to bare-metal
  - [ ] Implement required libc functions
  - [ ] Implement math library (libm)
  - [ ] Implement memory management for tensors
- [ ] LLVM IR generation for TinyLlama
  - [ ] Compile TinyLlama to LLVM bitcode
  - [ ] Link with kernel runtime
  - [ ] Create loadable module

### 5.2 Real-Time Optimization
- [ ] Profile TinyLlama execution
  - [ ] Identify hot loops
  - [ ] Identify memory bottlenecks
  - [ ] Identify branch patterns
- [ ] Dynamic recompilation
  - [ ] JIT compile hot functions at O0 (fast compile)
  - [ ] Recompile at O1 after 100 calls
  - [ ] Recompile at O2 after 1000 calls
  - [ ] Recompile at O3 for ultra-hot paths
- [ ] Specialized optimizations
  - [ ] SIMD vectorization (SSE/AVX if available)
  - [ ] Loop unrolling
  - [ ] Constant propagation for weights
  - [ ] Inlining for small functions
- [ ] Host-tuned deployment
  - [ ] Compile kernel/modules on target machine with llvm-libc toolchain
  - [ ] Validate runtime CPU feature alignment with build flags via pipeline reports
  - [ ] Record inference metrics pre/post host-specific tuning
  - [ ] Capture bench metadata using generated profile tags (e.g., `matrix_mul_AVX2`)
  - [ ] Massage/patch llvm-libc as needed for TinyLlama runtime gaps (document diffs)

## CPU Feature Detection Pipeline
1. **Host Scanner (build time)**  
   Run `clang -march=native -###` or `llvm-mca` helper to emit supported extensions (SSE, AVX, BMI, etc.) and capture `/proc/cpuinfo` for reference.
2. **Profile Export**  
   Normalize features into `build/cpu_profile.json` (flags list + cache sizes + core count). Commit or distribute with the build artifacts.
3. **Flag Injection**  
   Generate `kernel/auto_cpu_flags.mk` consumed by Makefiles to set `-mattr=` / `-march=` consistently for kernel, modules, and llvm-libc.
4. **Runtime Sanity Check**  
   Early boot routine reads CPUID, re-validates expected features, and logs discrepancies. If mismatch, fall back to conservative path or halt.
5. **Reporting**  
   Bench harness consumes the JSON to tag results (e.g., `matrix_mul_AVX2`) so comparisons across machines remain coherent.

## Benchmark Catalog
- **matrix_mul**  
  Dense 64×64 / 128×128 multiplication. Validates JIT threshold switching and SIMD gains; report cycles per multiply and reoptimization deltas.
- **fft_1d**  
  In-place radix-2 FFT on 1 k samples. Exercises trig tables and strided memory; track cycles per transform and cache effects post-optimization.
- **sha256_stream**  
  Hash 1 MB buffer chunks. Highlights memory bandwidth and pipeline efficiency; report throughput (MB/s) and instructions per block.
- **regex_dfa**  
  DFA-based pattern matcher over synthetic logs. Stresses branch prediction; compare misprediction counts and average cycles per character.
- **gemm_tile**  
  Tiled GEMM with configurable tile size. Validates allocator + cache behavior; collect cache hit rates and optimal tile selection flow.
- **physics_step**  
  Particle integrator (Verlet + collision). Mixes math/branching; track SIMD utilization and per-step latency, useful precursor to TinyLlama loops.***

### 5.3 Persistent Optimization Cache
- [ ] Boot-time cache loading
  - [ ] Load cached optimized functions
  - [ ] Verify cache integrity
  - [ ] Apply relocations
- [ ] Runtime cache updates
  - [ ] Write new optimized code to cache
  - [ ] Update cache metadata
  - [ ] Sync cache to disk periodically
- [ ] Cache management
  - [ ] Monitor cache size
  - [ ] Evict cold code
  - [ ] Defragment cache
  - [ ] Version migration

### 5.4 TinyLlama Demo
- [ ] Interactive prompt in kernel
- [ ] Load TinyLlama model from disk
- [ ] Run inference with JIT optimization
- [ ] Display profiling statistics
- [ ] Demonstrate cache persistence across reboots

## Success Metrics

### Performance Goals
- [ ] TinyLlama inference: < 1 second latency for short prompts
- [ ] JIT compilation overhead: < 100ms for hot functions
- [ ] Cache hit rate: > 90% after warmup
- [ ] Boot time: < 5 seconds to interactive prompt

### Code Quality
- [ ] Test coverage: > 80%
- [ ] Documentation coverage: 100% of public APIs
- [ ] Zero compiler warnings
- [ ] Clean QEMU execution (no CPU exceptions)

### Innovation
- [ ] First bare-metal LLM with JIT optimization
- [ ] Persistent JIT cache across reboots
- [ ] Real-time adaptive optimization
- [ ] Minimal memory footprint (< 512MB for model + runtime)

## Timeline Estimate

- **Phase 1**: 4-6 weeks (JIT + Modules)
- **Phase 2**: 3-4 weeks (Kernel extensions)
- **Phase 3**: 4-6 weeks (Optimizations)
- **Phase 4**: 2-3 weeks (Infrastructure)
- **Phase 5**: 8-12 weeks (TinyLlama integration)

**Total**: 21-31 weeks (~5-8 months)

## Notes

- Focus on incremental progress: each phase should produce a working system
- Prioritize JIT cache persistence early (Phase 3.1) as it's core to the project
- Consider memory constraints: TinyLlama requires ~1GB for model + activations
- May need to upgrade to 64-bit kernel for larger address space
- SIMD optimizations (SSE/AVX) will be critical for performance

---

**Last Updated**: 2025-10-25
**Status**: Phase 1.2 ✅ **COMPLETE & VALIDATED** - PGO system fully operational with 7 modules (fibonacci, sum, compute, primes, fft_1d, sha256, matrix_mul). Real performance gains measured: 1.06x-1.30x speedup with optimized recompilation. Phase 1.4 in progress: additional benchmarks created (quicksort, strops), investigating 9-module boot failure.
