# Architecture Decisions - BareFlow

## 🎯 Vision Principale

**BareFlow = Unikernel Ring 0 + LLVM JIT Runtime + llvm-libc**

Application unique (TinyLlama) avec compilation JIT LLVM à la volée pour optimisation adaptative sans downtime.

## Core Design Principles

### 🔥 RUNTIME JIT OPTIMIZATION - On-the-Fly Recompilation

**Critical Decision**: JIT optimization happens **AT RUNTIME**, not offline!

**Stack Complet**:
- **LLVM Bitcode**: Modules chargés au format .bc
- **LLVM JIT**: Compilation O0→O1→O2→O3 au runtime
- **llvm-libc**: Libc freestanding (string, math) pour bare-metal
- **Profiling**: rdtsc pour détection chemins chauds

**Current Status**:
- ❌ Current PGO: Offline recompilation (host-side with Python scripts)
- ✅ Target: Real-time JIT in kernel (LLVM OrcJIT or custom JIT)

**Real JIT Workflow**:
```
1. Load module as LLVM bitcode (.bc)
2. JIT compile to native code (O0 - fast compile)
3. Profile execution with rdtsc
4. Hot function detected (threshold reached)
5. → Trigger background recompilation (O1/O2/O3)
6. → Swap code pointer atomically
7. → Continue execution with optimized code
```

**Why This Matters**:
- Zero downtime for model inference
- Adaptive optimization based on actual workload
- No manual profiling/recompilation cycle
- Cache optimization converges automatically

**Implementation Path**:
- Phase 3.1: In-kernel LLVM OrcJIT integration
- Phase 3.2: Hot function detection + recompile triggers
- Phase 3.3: Code cache with atomic pointer swaps

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

## Current Phase Focus

**Phase 2.1**: Disk I/O ✅ (95% complete)
- FAT16 filesystem driver ✅
- Module loading from disk ✅
- PGO cache persistence to disk ⏳

**Phase 2.3**: Additional Drivers
- Serial port ✅ (already done for profiling)
- Timer interrupt ✅ (already done)
- Disk driver ✅ (FAT16 done)
- Network (optional, for remote profiling)

**Phase 3**: Performance Optimization
- PGO workflow ✅ (already functional)
- Multicore work distribution
- SIMD vectorization
- Cache-aware data structures

**Phase 5**: TinyLlama Integration (Final Goal)
- Single inference engine
- Multicore tensor operations
- JIT-optimized hot paths
- Zero-copy memory management

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

**Last Updated**: 2025-10-25
**Status**: Unikernel design confirmed, scheduler removed from roadmap
