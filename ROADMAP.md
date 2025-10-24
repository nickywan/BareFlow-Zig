# Fluid OS - Project Roadmap

## Vision
A bare-metal unikernel capable of running TinyLlama with real-time JIT optimization and persistent optimization cache across reboots.

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

### 3.2 Advanced Profiling
- [ ] Hardware performance counters (PMU)
  - [ ] Branch prediction statistics
  - [ ] Cache miss rates
  - [ ] TLB misses
- [ ] Sampling-based profiling
- [ ] Flame graph generation (export data)
- [ ] Real-time optimization triggers

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
