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

### 1.1 JIT Bare-Metal Integration
- [ ] Implement minimal C++ runtime for bare-metal
  - [ ] Operator new/delete with custom allocator
  - [ ] Basic exception handling stubs (or -fno-exceptions)
  - [ ] RTTI stubs (or -fno-rtti)
  - [ ] Static constructor/destructor support
- [ ] Port LLVM LLJIT to bare-metal
  - [ ] Memory allocator integration
  - [ ] Symbol resolver for kernel functions
  - [ ] JIT execution engine initialization
- [ ] JIT-AOT hybrid system
  - [ ] Load AOT modules as fallback
  - [ ] JIT compile hot functions on demand
  - [ ] Profile-guided optimization thresholds
- [ ] Benchmark (JIT smoke test): add `matrix_mul` module, capture baseline vs reoptimized cycles
- [ ] Early auto-optimization loop
  - [ ] Implement call-count thresholds on `matrix_mul`
  - [ ] Trigger recompilation at +100/+1000 calls (O0 → O1 → O2)
  - [ ] Log cycle deltas before/after
- [ ] Address Codex revue findings
  - [ ] Implement aligned allocation strategy in `jit_allocator` (padding vs base realign)
  - [ ] Fix global destructor order in `cxx_runtime` to match Itanium ABI
  - [ ] Return positive signal on successful `jit_auto_optimize` recompilations

### 1.2 Module System Improvements
- [ ] Dynamic module loading from disk
  - [ ] FAT16/FAT32 filesystem support (read-only)
  - [ ] Module loader from disk sectors
  - [ ] Module signature verification
- [ ] LLVM bitcode module support
  - [ ] Load .bc files from disk
  - [ ] JIT compile bitcode modules
  - [ ] Link with kernel runtime
- [ ] Enhanced profiling metrics
  - [ ] Per-function profiling
  - [ ] Call graph tracking
  - [ ] Cache hit/miss statistics
  - [ ] Memory allocation tracking
- [ ] Benchmark (profiling focus): run `fft_1d` module to validate call-count triggers
- [ ] Benchmark (memory-bound): run `sha256_stream` module to track throughput gains
- [ ] Auto-optimization iteration
  - [ ] Integrate profiling dashboards for bench modules
  - [ ] Demonstrate improvement timeline (initial vs reoptimized) in logs
- [ ] Toolchain alignment
  - [ ] Establish host-side build flow tuned for target CPU (SSE/AVX flags via clang/LLVM 18)
  - [ ] Integrate llvm-libc snapshot for freestanding builds
  - [ ] Implement CPU feature detection pipeline (see "CPU Feature Detection Pipeline")
  - [ ] Script `tools/gen_cpu_profile.py` to emit `build/cpu_profile.json`
  - [ ] Auto-generate `kernel/auto_cpu_flags.mk` from detected capabilities
  - [ ] Prototype bench metadata tags (e.g., `matrix_mul_AVX2`) using generated profiles
- [ ] Resolve Codex revue questions
  - [ ] Decide alignment policy for allocator fix (document chosen approach)
  - [ ] Define minimal deliverables for 1.2 (disk modules vs LLVM bitcode)
  - [ ] Specify target `fluid.img` size and default JIT pool allocations

## Phase 2: Kernel Extensions

### 2.1 Interrupt Handling
- [ ] IDT (Interrupt Descriptor Table) setup
- [ ] PIC (Programmable Interrupt Controller) configuration
- [ ] Timer interrupt (IRQ 0)
- [ ] Keyboard interrupt (IRQ 1)
- [ ] Exception handlers (divide by zero, page fault, etc.)

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

**Last Updated**: 2025-10-24
**Status**: Phase 1 in progress (bootloader fixed, starting JIT integration)
