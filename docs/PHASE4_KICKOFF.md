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
- FULL LLVM 18 integration (118MB - this is DESIRED!)
- Persistence des optimisations (FAT16)

---

## 📋 Phase 4 - Plan Détaillé

### Session 23-24: LLVM 18 Integration Setup (2-3 jours)

**Objectif**: Intégrer FULL LLVM 18 (118MB) pour auto-optimization

**⚠️ CRITICAL**: Use COMPLETE LLVM 18, not minimal build!

**Build Configuration** (Full Features)
```bash
# Use standard LLVM 18 from package manager
sudo apt install llvm-18-dev clang-18 llvm-18-tools

# OR build from source with ALL features
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_ENABLE_PROJECTS="clang" \
  -DLLVM_ENABLE_RTTI=ON \
  ../llvm
```

**Why FULL LLVM (118MB)?**
- ✅ Complete optimization passes (O0→O1→O2→O3)
- ✅ Interpreter + OrcJIT + all backends
- ✅ Full profiling and analysis tools
- ✅ Maximum auto-optimization capability
- ✅ Size reduction comes FROM convergence, not initial constraints

**Tasks**:
1. ⬜ Install/build FULL LLVM 18 (118MB)
2. ⬜ Verify all optimization passes available
3. ⬜ Test with Phase 3 validation suite
4. ⬜ Confirm Interpreter + OrcJIT working
5. ⬜ Document bare-metal integration requirements

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

**Objectif**: Créer image bootable avec FULL LLVM

**Architecture**:
```
tinyllama_jit.img (~118MB - Boot 1, this is DESIRED!)
├── Stage 1 (512B)
├── Stage 2 (4KB)
└── Kernel + LLVM (~118MB)
    ├── tinyllama app IR (~100KB)
    ├── LLVM FULL runtime (~118MB - ALL features!)
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
1. ⬜ Modifier bootloader pour ~118MB (full LLVM)
2. ⬜ Créer section .ir dans binary
3. ⬜ Implémenter IR loader
4. ⬜ Hook profiling dans interpreter
5. ⬜ Tester boot + execution with FULL LLVM

---

### Session 29-30: Persistence (2-3 jours)

**Objectif**: Sauvegarder optimisations sur disque

**FAT16 Integration**:
```
/boot/
  ├── tinyllama_base.img      # Image initiale ~118MB (FULL LLVM)
  └── snapshots/
      ├── boot_001.snapshot   # Après 10 boots (~118MB)
      ├── boot_100.snapshot   # JIT O0-O3 appliqué (~30MB)
      └── boot_500.snapshot   # Dead code éliminé (~10MB)
      └── boot_1000.snapshot  # Native export (~2-5MB)
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
- [ ] FULL LLVM 18 integration (118MB is OK!)
- [ ] Boot avec LLVM complet en bare-metal
- [ ] Execute simple IR function with Interpreter
- [ ] JIT compile et execute (OrcJIT)
- [ ] Measure speedup vs interpreter (target 399×)

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

| Métrique | Boot 1 | Boot 1000+ | Comment mesurer |
|----------|--------|------------|-----------------|
| **Image size** | ~118MB | ~2-5MB | `du -sh tinyllama_jit.img` |
| **Boot time** | ~10-30s | <1s | rdtsc timestamps |
| **Interpreter perf** | ~500× slower | N/A | vs AOT baseline |
| **JIT compile time** | <100ms | <10ms | Per function |
| **JIT perf** | <2× slower AOT | ~AOT native | After convergence |
| **Memory usage** | ~200MB | ~32MB | Custom allocator stats |

---

## 🔧 Toolchain Setup

### Required
```bash
# Use pre-built LLVM 18 (recommended)
sudo apt install llvm-18-dev clang-18 llvm-18-tools

# OR build from source (FULL build, not minimal!)
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout release/18.x

# Build COMPLETE LLVM with all features
mkdir build && cd build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="X86" \
  -DLLVM_ENABLE_PROJECTS="clang" \
  ../llvm
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

### Risque 1: Bootloader limitations pour 118MB
**Impact**: Impossible de charger image complète
**Probabilité**: Moyenne
**Mitigation**:
- Extend bootloader LBA loading (currently limited)
- Use proper memory mapping (0x100000+ region)
- Consider multi-stage loading if needed

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
2. ✅ Correct project philosophy (NO size optimization!)
3. ⬜ Verify FULL LLVM 18 installation
4. ⬜ Test all optimization passes available
5. ⬜ Create bare-metal integration plan

### Week 1 (Sessions 23-24)
- FULL LLVM 18 ready (118MB - this is correct!)
- All optimization passes validated
- Tests Phase 3 passing with full LLVM
- Bare-metal port strategy documented

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