# Phase 4: Bare-Metal JIT Integration - Kickoff

**Date de début**: 2025-10-26
**Sessions prévues**: 23-30 (8 sessions)
**Objectif**: Intégrer LLVM JIT dans le bare-metal unikernel

---

## 🎯 Objectif Global

Passer de la validation userspace (Phase 3) à l'implémentation bare-metal complète du système JIT "Grow to Shrink".

**Résultat attendu**: Un unikernel bootable qui démarre avec LLVM complet, profile tout, JIT compile les hot paths, et peut exporter un binaire minimal.

---

## 📊 État Actuel (Post-Phase 3)

### ✅ Ce qui est VALIDÉ
- **Stratégie "Grow to Shrink"**: Prouvée end-to-end en userspace
- **399× speedup**: Interpreter → JIT démontré
- **99.83% dead code**: Identifié dans LLVM
- **6000× reduction**: Possible (118MB → 20KB)
- **Tiered compilation**: O0→O3 automatique fonctionnel

### 🏗️ Ce qui existe ACTUELLEMENT
- **Unikernel AOT**: 28KB (kernel_lib 15KB + tinyllama 13KB)
- **Profiling**: Cycle-accurate avec rdtsc
- **Bootloader**: 2-stage, protected mode, A20 enabled
- **I/O**: VGA, Serial, Keyboard fonctionnels
- **Tests Phase 3**: 17 programmes de validation

### 🚧 Ce qui MANQUE pour Phase 4
- LLVM JIT intégré dans bare-metal
- Allocateur custom pour JIT
- Runtime C++ minimal (no exceptions, no RTTI)
- Custom LLVM build (2-5MB vs 118MB)
- Persistence des optimisations (FAT16)

---

## 📋 Phase 4 - Plan Détaillé

### Session 23-24: Custom LLVM Build (2-3 jours)

**Objectif**: Réduire LLVM de 118MB à 2-5MB

**Option A - Custom LLVM Build** (Recommandé)
```bash
# Configuration minimale
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_ENABLE_PROJECTS="" \
  -DLLVM_INCLUDE_TOOLS=OFF \
  -DLLVM_BUILD_TOOLS=OFF \
  -DLLVM_ENABLE_TERMINFO=OFF \
  -DLLVM_ENABLE_ZLIB=OFF \
  -DLLVM_ENABLE_LIBXML2=OFF \
  ../llvm
```

**Option B - Alternative JIT** (Fallback)
- **QBE**: Tiny backend (30KB), C99
- **Cranelift**: Rust-based, ~2MB
- **MIR**: Minimal IR JIT, ~500KB

**Critères de décision**:
1. Taille finale (<5MB)
2. Complexité d'intégration
3. Performance (doit être proche AOT)
4. Support bare-metal

**Tasks**:
1. ✅ Rechercher options de build LLVM minimal
2. ⬜ Tester build avec X86 only
3. ⬜ Mesurer taille résultante
4. ⬜ Valider avec tests Phase 3
5. ⬜ Décision: LLVM custom ou alternative?

---

### Session 25-26: Bare-Metal Port (3-4 jours)

**Objectif**: Porter LLVM OrcJIT vers bare-metal

**Challenges**:
1. **Allocateur**: malloc/free bare-metal pour JIT
2. **C++ runtime**: Pas d'exceptions, pas de RTTI
3. **Dependencies**: Remplacer syscalls par kernel_lib
4. **Threading**: Pas de threads (single-core initial)

**Approche**:
```cpp
// kernel_lib/jit/llvm_allocator.c
void* jit_alloc(size_t size) {
    // Bump allocator pour JIT code
    // Pool séparé de malloc général
}

// kernel_lib/jit/llvm_wrapper.cpp
extern "C" {
    void* jit_compile_ir(const char* ir, size_t len) {
        // Wrapper bare-metal pour LLVM OrcJIT
    }
}
```

**Tasks**:
1. ⬜ Implémenter JIT allocator
2. ⬜ Créer wrapper C bare-metal
3. ⬜ Stub C++ runtime minimal
4. ⬜ Tester compilation simple IR
5. ⬜ Intégrer avec kernel_lib

---

### Session 27-28: Boot Integration (2-3 jours)

**Objectif**: Créer image bootable 60MB avec LLVM

**Architecture**:
```
tinyllama_jit.img (60MB)
├── Stage 1 (512B)
├── Stage 2 (4KB)
└── Kernel + LLVM (~60MB)
    ├── tinyllama app IR (~100KB)
    ├── LLVM JIT runtime (~2-5MB)
    └── kernel_lib (~15KB)
```

**Boot sequence**:
1. Stage 2 loads 60MB to 0x100000
2. Verify FLUD signature
3. Jump to kernel entry
4. Initialize JIT runtime
5. Load app IR from embedded section
6. Execute IR with interpreter
7. Profile function calls
8. JIT compile hot functions

**Tasks**:
1. ⬜ Modifier bootloader pour 60MB
2. ⬜ Créer section .ir dans binary
3. ⬜ Implémenter IR loader
4. ⬜ Hook profiling dans interpreter
5. ⬜ Tester boot + execution

---

### Session 29-30: Persistence (2-3 jours)

**Objectif**: Sauvegarder optimisations sur disque

**FAT16 Integration**:
```
/boot/
  ├── tinyllama_base.img      # Image initiale 60MB
  └── snapshots/
      ├── boot_001.snapshot   # Après 10 boots
      ├── boot_100.snapshot   # JIT O0-O3 appliqué
      └── boot_500.snapshot   # Dead code éliminé
```

**Snapshot format**:
```c
struct jit_snapshot {
    uint32_t magic;           // "SNAP"
    uint32_t version;
    uint32_t boot_count;
    uint32_t num_functions;
    struct {
        char name[32];
        uint32_t offset;      // Dans section code
        uint32_t size;
        uint8_t opt_level;    // 0-3
    } functions[];
    uint8_t native_code[];
};
```

**Tasks**:
1. ⬜ Implémenter FAT16 write
2. ⬜ Créer snapshot format
3. ⬜ Serializer JIT code
4. ⬜ Loader snapshots at boot
5. ⬜ Version management

---

## 🎯 Success Criteria Phase 4

### Minimum Viable Product (MVP)
- [ ] Custom LLVM build ≤5MB
- [ ] Boot avec LLVM en bare-metal
- [ ] Execute simple IR function
- [ ] JIT compile et execute
- [ ] Measure speedup vs interpreter

### Target Complet
- [ ] Tiered compilation (O0→O3)
- [ ] Profiling automatique
- [ ] Snapshot persistence
- [ ] 10× speedup après 100 boots
- [ ] Dead code detection working

### Stretch Goals
- [ ] Multiple optimization strategies
- [ ] Adaptive thresholds
- [ ] Cross-function inlining
- [ ] Vector instruction generation

---

## 📏 Métriques à Tracker

| Métrique | Target | Comment mesurer |
|----------|--------|-----------------|
| **LLVM size** | 2-5MB | `du -sh libLLVM*.a` |
| **Boot time** | <10s | rdtsc timestamps |
| **Interpreter perf** | ~500× slower | vs AOT baseline |
| **JIT compile time** | <100ms | Per function |
| **JIT perf** | <2× slower AOT | After warmup |
| **Memory usage** | <128MB | Custom allocator stats |

---

## 🔧 Toolchain Setup

### Required
```bash
# LLVM source
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout release/18.x

# Build minimal LLVM
mkdir build && cd build
cmake -G Ninja [OPTIONS] ../llvm
ninja
```

### Development Loop
```bash
# 1. Modify JIT wrapper
vim kernel_lib/jit/llvm_wrapper.cpp

# 2. Rebuild
cd kernel_lib && make

# 3. Test userspace first
cd ../tests/phase4
./test_bareflow_jit

# 4. If OK, rebuild unikernel
cd ../../tinyllama
make rebuild

# 5. Boot test
make run
```

---

## ⚠️ Risques & Mitigations

### Risque 1: LLVM trop gros même minimal
**Impact**: Impossible de booter 60MB
**Probabilité**: Moyenne
**Mitigation**:
- Try QBE/Cranelift alternatives
- Split LLVM en modules chargés à la demande

### Risque 2: C++ runtime dependencies
**Impact**: Crashes en bare-metal
**Probabilité**: Haute
**Mitigation**:
- Stub tout avec extern "C"
- Pas d'exceptions (-fno-exceptions)
- Pas de RTTI (-fno-rtti)
- Custom new/delete operators

### Risque 3: Performance regression
**Impact**: JIT plus lent que AOT
**Probabilité**: Faible (validé Phase 3)
**Mitigation**:
- Continuous benchmarking
- Compare avec AOT baseline
- Profiling overhead <5%

### Risque 4: Complexity explosion
**Impact**: Impossible à maintenir
**Probabilité**: Moyenne
**Mitigation**:
- Start simple (interpreter only)
- Incremental features
- Document everything
- Regular refactoring

---

## 📚 Resources

### Documentation
- `docs/phase3/*.md` - Validation results
- `ARCHITECTURE_UNIKERNEL.md` - System design
- `LLVM_PIPELINE.md` - Optimization strategy

### Code Examples
- `tests/phase3/test_llvm_interpreter.cpp` - Interpreter baseline
- `tests/phase3/test_tiered_jit.cpp` - Tiered compilation
- `archive/monolithic_kernel/kernel/jit_llvm18.cpp` - Old JIT wrapper

### External
- LLVM OrcJIT Tutorial: https://llvm.org/docs/tutorial/
- Bare-metal C++: https://wiki.osdev.org/C++
- QBE Backend: https://c9x.me/compile/

---

## 🚀 Next Actions

### Immediate (Session 23)
1. ✅ Read this kickoff document
2. ⬜ Research LLVM minimal build options
3. ⬜ Create build script for custom LLVM
4. ⬜ Test build and measure size
5. ⬜ Document findings in `docs/PHASE4_LLVM_BUILD.md`

### Week 1 (Sessions 23-24)
- Custom LLVM build working
- Size ≤5MB validated
- Tests Phase 3 passing with custom build
- Decision document created

### Week 2 (Sessions 25-26)
- Bare-metal port complete
- Simple IR execution working
- JIT compilation functional
- Integration with kernel_lib done

---

**Créé**: 2025-10-26
**Maintainer**: Claude Code Assistant
**Human**: @nickywan
**Status**: 🚀 **PHASE 4 STARTING NOW!**