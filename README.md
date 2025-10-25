# BareFlow - LLVM JIT Unikernel

**Bare-metal unikernel with runtime LLVM JIT optimization and llvm-libc**

## Vision

BareFlow est un **unikernel Ring 0** qui exécute une seule application (TinyLlama LLM) avec **compilation JIT LLVM à la volée**. Le kernel profile l'exécution en temps réel et recompile les chemins chauds avec des niveaux d'optimisation croissants (O0→O1→O2→O3) sans interruption.

### Core Principles

- **Unikernel**: Application unique, pas de multitâche, exécution Ring 0
- **Runtime JIT**: LLVM bitcode → optimisation adaptative au runtime
- **llvm-libc**: Subset libc freestanding pour bare-metal
- **Multicore**: Parallélisme de données pour opérations tensorielles (pas de tâches)
- **Zero Downtime**: Swap atomique de pointeurs pendant réoptimisation

## Quick Start

```bash
# Build kernel
make clean && make

# Run in QEMU
make run

# Test JIT (userspace)
make -f Makefile.jit test-interface
```

## Architecture

```
┌─────────────────────────────────────────────┐
│  TinyLlama Inference (Application Unique)   │
├─────────────────────────────────────────────┤
│  Runtime Profiler (rdtsc)                   │
│    ↓ Détecte fonctions chaudes             │
├─────────────────────────────────────────────┤
│  LLVM JIT Compiler                          │
│    • Load LLVM bitcode (.bc)                │
│    • Compile O0 (baseline rapide)           │
│    • Recompile O1/O2/O3 quand chaud         │
│    • Swap atomique (zero downtime)          │
├─────────────────────────────────────────────┤
│  Kernel Services                            │
│    • FAT16 filesystem                       │
│    • Module loader                          │
│    • JIT allocator                          │
│    • llvm-libc                              │
├─────────────────────────────────────────────┤
│  Hardware (x86-32, Ring 0)                  │
└─────────────────────────────────────────────┘
```

## Status

- ✅ Phase 1: Module system + profiling
- ✅ Phase 2.1: FAT16 filesystem
- ⚡ Phase 3.1: Bitcode modules (60%)
- 🔄 Phase 3.2: Micro-JIT (20%)
- 📋 Phase 5: TinyLlama

**Kernel**: 82KB, 12 modules

## Runtime JIT Workflow

1. Load bitcode (.bc) depuis disque
2. JIT compile à O0 (rapide)
3. Profile avec rdtsc
4. Détecte fonction chaude (100 appels)
5. Recompile à O1 en background
6. Swap atomique pointeur
7. Répète: O2 à 1000, O3 à 10000 appels

## Documentation

- **ROADMAP.md**: Roadmap complète
- **CLAUDE_CONTEXT.md**: État développement
- **RUNTIME_JIT_PLAN.md**: Plan JIT
- **ARCHITECTURE_DECISIONS.md**: Principes design

Voir ROADMAP.md pour détails complets.
