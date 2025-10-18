# 🔥 Fluid OS - Self-Optimizing Kernel

**Un unikernel avec LLVM JIT runtime qui s'optimise lui-même à chaud.**

## Quick Start

### Phase 1 (User-Space JIT) - DONE ✅
```bash
# Build module
clang-18 -c -emit-llvm -O2 -ffreestanding -nostdlib \
    kernel/stdlib.c -o kernel/stdlib.bc
clang-18 -c -emit-llvm -O2 -ffreestanding -nostdlib \
    app/test.c -o app/test.bc
llvm-link-14 kernel/stdlib.bc app/test.bc -o fluid_module.bc

# Test JIT
cd jit_test
./jit_profiler ../fluid_module.bc
```

### Phase 2 (Kernel Bare Metal) - IN PROGRESS 🚧
```bash
cd fluid_kernel
make
make run
```

## Documentation

Voir [PROJECT.md](PROJECT.md) pour documentation complète.

## Status

- ✅ Phase 1: POC User-Space (VALIDÉE)
- 🚧 Phase 2: Kernel Bare Metal (EN COURS)
- ⏳ Phase 3: llama.cpp Integration (FUTUR)

## Architecture
```
Application (LLVM IR)
       ↕
Kernel Fluid (Ring 0)
  ├── LLVM ORC JIT
  ├── Profiler Runtime
  └── Re-JIT Optimizer
```

**Innovation**: Kernel qui se JIT lui-même pour s'adapter au workload.

## License

Educational/Research - Not production ready
