# BareFlow - Project Roadmap

**Last Updated**: 2025-11-01 (Zig Migration Start)
**Architecture**: Self-Optimizing Unikernel with "Grow to Shrink" Strategy
**Current Phase**: Phase 6 - Zig Kernel Migration
**Target Platforms**: x86_64 (primary), aarch64 (secondary)

---

## 🎯 Vision: "Grow to Shrink" Strategy

**BareFlow = Self-Optimizing Program**

> "The program is self-sufficient, embeds everything initially (60MB with LLVM),
> self-profiles, self-optimizes, and converges to a minimal binary (2-5MB)."

### Progressive Convergence Lifecycle

```
Boot 1-10:   [60-118MB] Full LLVM + app in IR  → Interpreted (slow, profiles everything)
Boot 10-100: [30-60MB]  Hot paths JIT O0→O3    → 10× faster
Boot 100-500:[10MB]     Dead code eliminated    → Relinked optimized
Boot 500+:   [2-5MB]    Pure native export      → LLVM removed, appliance mode
```

---

## 🔄 MAJOR ARCHITECTURE CHANGE: C → Zig Migration

### Why Migrate to Zig?

After 45 sessions fighting C issues:
- **malloc nightmares** (Sessions 31-36): Fixed addresses, BSS initialization, silent corruption
- **Return value bugs** (Sessions 37-45): Clang code generation errors, ABI mismatches
- **Runtime complexity**: 15KB of handwritten functions, constant reinvention

### Zig Solutions:
- ✅ **Built-in allocators**: FixedBuffer, Arena, General Purpose (no malloc hell)
- ✅ **Comptime safety**: Errors caught at compile-time, not runtime
- ✅ **LLVM native**: Better IR generation for JIT optimization
- ✅ **Freestanding first-class**: Designed for bare-metal from day one

**Decision Document**: See `ZIG_MIGRATION_STRATEGY.md`

---

## ✅ Completed Phases (Archived)

<details>
<summary>Phases 1-5: C Implementation (Click to expand)</summary>

### Phase 1-2: AOT Baseline Unikernel (Sessions 1-20)
- Unikernel architecture (28KB binary)
- Basic runtime library (kernel_lib.a 15KB)
- Zero syscall overhead achieved

### Phase 3: Hybrid Runtime Validation (Sessions 21-22)
- LLVM JIT validated (399× speedup)
- Tiered compilation (O0→O3)
- Dead code analysis (99.83% unused)
- Native export (6000× reduction)

### Phase 4: Bare-Metal Integration (Sessions 23-30)
- 64-bit runtime (kernel_lib_llvm.a 29KB)
- QEMU validation infrastructure
- C++ runtime implementation
- malloc investigation (deferred due to bugs)

### Phase 5: C Debugging Attempts (Sessions 31-45)
- Paging implementation (4-level, 2MB pages)
- Bump allocator workaround
- Multiple malloc debugging sessions
- **BLOCKED**: Clang code generation bugs

</details>

---

## 🚀 Phase 6: Zig Kernel Migration (CURRENT)

### Overview
Complete rewrite of kernel components in Zig while maintaining LLVM JIT strategy.

### Sessions 46-48: Kernel Core in Zig
**Status**: 🟡 IN PROGRESS

**Goals**:
- [ ] Minimal bootable Zig kernel (multiboot2)
- [ ] Serial I/O for debugging
- [ ] VGA text mode driver
- [ ] Basic heap allocator (FixedBufferAllocator)

**Tasks**:
1. [ ] Setup Zig build system for freestanding x86_64
2. [ ] Port boot entry (multiboot2 header)
3. [ ] Implement serial driver in Zig
4. [ ] Create VGA driver with comptime safety
5. [ ] Setup heap with FixedBufferAllocator
6. [ ] Validate in QEMU x86_64

**Success Criteria**:
- Boots in QEMU with "Hello from Zig!" message
- Serial output working
- Basic allocator functional
- No C dependencies remaining in kernel core

### Sessions 49-50: TinyLlama Port to Zig
**Status**: 📅 PLANNED

**Goals**:
- [ ] Load TinyLlama model (60MB)
- [ ] Model struct definitions in Zig
- [ ] Weight loading with safety checks
- [ ] Basic inference loop

**Tasks**:
1. [ ] Define model structures with packed structs
2. [ ] Implement weight loader with bounds checking
3. [ ] Port matrix operations to Zig
4. [ ] Add profiling hooks
5. [ ] Benchmark vs C version

### Sessions 51-52: LLVM JIT Integration
**Status**: 📅 PLANNED

**Goals**:
- [ ] LLVM ORC JIT from Zig
- [ ] Memory manager (RW→RX transitions)
- [ ] Symbol resolver
- [ ] Tiered compilation (O0→O3)

**Tasks**:
1. [ ] Create Zig bindings for LLVM ORC
2. [ ] Implement JIT memory manager
3. [ ] Setup symbol resolution
4. [ ] Add profiling-driven tiering
5. [ ] Test with simple IR modules

### Sessions 53-54: Profile-Guided Optimization
**Status**: 📅 PLANNED

**Goals**:
- [ ] Hot path detection
- [ ] Automatic vectorization
- [ ] CPU feature detection (AVX/NEON)
- [ ] Persistent optimization cache

**Tasks**:
1. [ ] Implement profiling infrastructure
2. [ ] Detect hot functions/loops
3. [ ] CPU capability detection (comptime where possible)
4. [ ] Specialization plan generation
5. [ ] Cache persistence to disk

---

## 🎯 Phase 7: ARM64 Port (Future)

### Sessions 55-58: Raspberry Pi Support
**Status**: 🔮 FUTURE

**Goals**:
- [ ] Boot on QEMU raspi3
- [ ] Real Raspberry Pi 4 support
- [ ] UART driver
- [ ] MMU and caches
- [ ] I/D cache flush for JIT

---

## 📊 Success Metrics

### Phase 6 (Zig Migration)
- [ ] Zero C code in kernel core
- [ ] Comptime safety for all drivers
- [ ] No malloc-related crashes
- [ ] Allocator usage < 1MB for kernel
- [ ] Boot time < 100ms in QEMU

### Phase 7 (ARM64)
- [ ] Boots on real Raspberry Pi 4
- [ ] JIT works with cache flush
- [ ] Performance parity with x86_64

### Ultimate Goal
- [ ] 2-5MB final binary after convergence
- [ ] Hardware-optimal performance
- [ ] Zero manual optimization required
- [ ] Works on x86_64 AND aarch64

---

## 📅 Timeline Estimates

| Phase | Sessions | Duration | Status |
|-------|----------|----------|---------|
| **Phase 1-5** (C attempts) | 1-45 | ✅ Complete | Archived (see details above) |
| **Phase 6** (Zig kernel) | 46-54 | 2-3 weeks | 🚀 **CURRENT** |
| **Phase 7** (ARM64 port) | 55-58 | 1-2 weeks | 🔮 Future |
| **Phase 8** (Production) | 59-65 | 2-3 weeks | 🔮 Future |

---

## 🛠️ Technical Stack

### Current (Phase 6+)
- **Language**: Zig (kernel), LLVM IR (applications)
- **Allocator**: Zig built-in allocators (FixedBuffer → GeneralPurpose)
- **JIT**: LLVM 18 ORC JIT
- **Runtime**: llvm-libc functions in IR (JIT-optimizable)
- **Targets**: x86_64-freestanding, aarch64-freestanding

### Build Tools
- **Zig**: 0.11.0 or newer
- **LLVM**: 18.x for JIT
- **QEMU**: For testing (x86_64, aarch64)
- **GRUB**: For bootable ISOs

---

## 📚 Key Documentation

### Migration & Strategy
- `ZIG_MIGRATION_STRATEGY.md` - Why and how we're moving to Zig
- `CLAUDE.md` - Multi-agent workflow and rules
- `CONTEXT_BAREFLOW.md` - Current state and decisions

### Architecture
- `README.md` - Vision and philosophy
- `ARCHITECTURE_UNIKERNEL.md` - Technical design

### Historical (Archived)
- `archive/docs/phase*/` - C implementation attempts
- `archive/docs/malloc*/` - malloc debugging saga

---

## ⚠️ Risks & Mitigations

### Risk 1: Zig learning curve
**Mitigation**: Start with simple drivers, leverage comptime for safety

### Risk 2: LLVM integration complexity
**Mitigation**: Use existing C bindings initially, migrate gradually

### Risk 3: ARM64 differences
**Mitigation**: Abstract platform-specific code early

### Risk 4: Performance regression
**Mitigation**: Continuous benchmarking, A/B testing

---

## 🎉 Expected Improvements with Zig

1. **Memory safety**: No more silent corruption
2. **Compile-time validation**: Catch errors before runtime
3. **Smaller code**: Zig's comptime reduces runtime code
4. **Better errors**: Explicit error handling, no NULL
5. **Cleaner architecture**: Built-in allocators, no malloc complexity
6. **Faster development**: No more weeks debugging malloc

---

**Project**: BareFlow - Self-Optimizing Unikernel
**Maintainer**: Claude Code Assistant & Sub-Agents
**Human**: @nickywan
**Status**: 🚀 Phase 6 - Zig Kernel Migration