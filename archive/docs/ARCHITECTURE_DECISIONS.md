# Architecture Decisions - BareFlow

## 🎯 Vision Principale (Updated Session 17)

**BareFlow = Self-Optimizing Unikernel Program**

> "Le kernel n'est plus qu'une bibliothèque d'accès au processeur, ce n'est pas un kernel
> (un cœur) mais juste un profiler qui aide le programme à fonctionner du mieux possible."

Application unique (TinyLlama) qui s'auto-profile et s'auto-optimise, liée statiquement avec une bibliothèque runtime minimale (kernel_lib.a ~20-30KB) pour accès direct au processeur.

**Architecture**: Single binary (~100KB) = Application + Runtime Library (kernel_lib.a)
- Application: TinyLlama inference + self-profiling + self-optimization
- Runtime Library: I/O (VGA, serial, keyboard) + Memory (malloc, string) + CPU (rdtsc, cpuid) + JIT (profiling, optimization)

**See**: [ARCHITECTURE_UNIKERNEL.md](ARCHITECTURE_UNIKERNEL.md), [LLVM_PIPELINE.md](LLVM_PIPELINE.md)

## Core Design Principles

### 🔥 LLVM-BASED SELF-OPTIMIZATION - Incremental Evolution

**Critical Decision**: Everything compiled with LLVM for coherent optimization!

**Stack Complet**:
- **Application**: TinyLlama inference (compiled with LLVM)
- **Runtime Library**: kernel_lib.a (compiled with LLVM)
- **llvm-libc**: Freestanding libc for bare-metal
- **Profiling**: rdtsc-based cycle counting
- **Optimization**: Self-profiling + self-optimization calls

**4-Phase Evolution** (see [LLVM_PIPELINE.md](LLVM_PIPELINE.md)):

1. **Phase 1: AOT Simple (IMMEDIATE)** ✅
   - Everything compiled to native code with LLVM LTO
   - Single binary ~100KB
   - Boot time ~0.5s
   - Self-profiling with jit_profile_begin/end

2. **Phase 2: Mini Interpreter**
   - Minimal native bootstrap (~10KB)
   - Interprets LLVM IR at runtime
   - Proof of concept

3. **Phase 3: Meta-Circular** 🎯 **VISION FINALE**
   - LLVM interpreter written in LLVM IR
   - Self-interprets, self-profiles, self-optimizes
   - JIT compiler written in LLVM IR
   - Profiler written in LLVM IR
   - Complete self-optimization stack

4. **Phase 4: Persistence**
   - Cache JIT-compiled native code to disk
   - Eliminate warmup after first run
   - Boot time ~0.5s with optimized code

**Why This Matters**:
- Coherent LLVM optimization across entire stack
- Self-optimization without external tools
- Meta-circular evaluation (PyPy, Truffle/Graal inspiration)
- Zero overhead (direct calls, no syscalls)

### ⚠️ NO MULTITASKING - Single Application Unikernel

**Decision**: BareFlow is a **unikernel** running in **Ring 0** with a single application.

**Rationale**:
- JIT optimization at runtime for ONE dedicated workload (TinyLlama inference)
- No context switching overhead
- No process isolation needed
- Direct hardware access
- Maximum performance for single-threaded ML inference

**What this means**:
- ❌ NO scheduler (no task switching)
- ❌ NO multitasking (one app only)
- ❌ NO user space / kernel space separation
- ✅ YES multicore support (data parallelism for inference)
- ✅ YES interrupts (timer, keyboard, disk I/O)
- ✅ YES dynamic module loading (for ML model layers)

### Multicore Strategy

**Approach**: Data parallelism, NOT task parallelism

```
CPU 0 (BSP):           CPU 1-N (APs):
- Boot kernel          - Matrix multiplication chunks
- JIT compilation      - FFT parallel blocks
- Module loading       - Tensor operations
- I/O management       - Batch inference
```

**Use cases**:
- Split matrix operations across cores
- Parallel tensor computations
- Multi-batch inference (not multi-process)
- SIMD + multicore for maximum throughput

### Phase 2.2 Scheduler - SKIP IT!

**Original Roadmap** says:
```
### 2.2 Basic Scheduler
- [ ] Task structure definition
- [ ] Round-robin scheduler
- [ ] Context switching
- [ ] Cooperative multitasking
```

**Actual Decision**: ❌ **SKIP Phase 2.2 entirely**

We don't need a scheduler. Replace with:

### 2.2 Multicore Bootstrap (Replacement)
- [ ] AP (Application Processor) startup code
- [ ] APIC (Advanced Programmable Interrupt Controller) setup
- [ ] Per-core data structures
- [ ] Core affinity for module execution
- [ ] Work distribution (not task distribution)

## Current Phase Focus (Session 17-18)

**Phase 6: Unikernel Refactor** 🔥 **CRITICAL PATH**

**Week 1 (Session 18)**: Extract kernel_lib.a
- [ ] Create kernel_lib/ directory structure
- [ ] Extract I/O: VGA, serial, keyboard → kernel_lib/io/
- [ ] Extract Memory: malloc, string → kernel_lib/memory/
- [ ] Extract CPU: rdtsc, cpuid → kernel_lib/cpu/
- [ ] Extract JIT: profiling, optimization → kernel_lib/jit/
- [ ] Build kernel_lib.a (~20-30KB)

**Week 2 (Session 19-20)**: Build tinyllama stub
- [ ] Create tinyllama/ directory
- [ ] Implement main.c with self-profiling
- [ ] Link with kernel_lib.a
- [ ] Generate tinyllama_bare.elf

**Week 3 (Session 21)**: Bootable image
- [ ] Update linker script
- [ ] Create fluid_llama.img
- [ ] Test boot in QEMU

**Weeks 4-6 (Sessions 22-25)**: TinyLlama integration
- [ ] Port inference code
- [ ] Validate correctness
- [ ] Optimize hot paths

**See**: [NEXT_SESSION_UNIKERNEL.md](NEXT_SESSION_UNIKERNEL.md) for Session 18 detailed plan

## Implementation Notes

### Interrupts (YES)
- Timer: for profiling timestamps
- Keyboard: for interactive debugging
- Disk: for async I/O (future)
- APIC: for multicore coordination

### Memory Management (YES, but simple)
- Single address space (no virtual memory)
- JIT allocator for code generation
- Heap for tensors and modules
- No process isolation
- No memory protection (Ring 0)

### Concurrency Model
- Single-threaded control flow
- Data-parallel execution (multicore)
- No locks (no concurrent writes)
- Lock-free queues for core communication (future)

---

**Last Updated**: 2025-10-25 (Session 17)
**Status**: Self-optimizing unikernel architecture confirmed, Phase 6 (Unikernel Refactor) in progress
**Architecture**: Option 2 - Single binary with kernel_lib.a runtime library
**Vision**: Meta-circular LLVM self-optimization (4-phase evolution)
