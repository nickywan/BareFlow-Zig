# BareFlow - LLVM Pipeline Complete (4 Phases vers Meta-Circular)

**Vision** : LLVM qui s'auto-interprète, s'auto-profile, et s'auto-optimise

> "Le code natif au boot ne doit servir qu'au strict nécessaire : booter la machine,
> puis booter la compilation JIT de l'interpréteur LLVM (codé en LLVM), le processeur
> JIT (en LLVM), le profiler JIT (en LLVM). Le warmup est long mais ensuite grâce à
> la persistance, le warmup sera plus rapide."

---

## 🎯 Vision Finale : Meta-Circular Evaluation

```
┌──────────────────────────────────────────────────────┐
│ BOOT NATIF (~10KB) - Minimal ASM                     │
│  - stage1.asm (512 bytes)                            │
│  - stage2.asm (4KB)                                  │
│  - llvm_bootstrap.bin (5KB natif)                    │
│    → Mini LLVM interpreter natif                     │
└──────────────────────────────────────────────────────┘
                    ↓ load & execute
┌──────────────────────────────────────────────────────┐
│ LLVM INTERPRETER (interpreter.bc) - LLVM IR          │
│  - Écrit 100% en LLVM IR                             │
│  - S'auto-interprète (meta-circular!)                │
│  - Profile lui-même en exécutant                     │
│  - JIT compile ses propres hot paths                 │
│  - Devient progressivement natif                     │
└──────────────────────────────────────────────────────┘
                    ↓ interprète
┌──────────────────────────────────────────────────────┐
│ JIT COMPILER (jit_compiler.bc) - LLVM IR             │
│  - Écrit en LLVM IR                                  │
│  - Compile LLVM IR → code natif                      │
│  - S'optimise lui-même aussi                         │
└──────────────────────────────────────────────────────┘
                    ↓ compile
┌──────────────────────────────────────────────────────┐
│ PROFILER (profiler.bc) - LLVM IR                     │
│  - Écrit en LLVM IR                                  │
│  - Profile TOUT (interpreter + JIT + app)            │
│  - S'optimise lui-même                               │
└──────────────────────────────────────────────────────┘
                    ↓ profile & optimize
┌──────────────────────────────────────────────────────┐
│ APPLICATION (tinyllama.bc) - LLVM IR                 │
│  - TinyLlama inference                               │
│  - Exécuté par interpreter auto-optimisé             │
│  - Profiling automatique                             │
│  - JIT compilation automatique                       │
└──────────────────────────────────────────────────────┘
                    ↓ persistance
┌──────────────────────────────────────────────────────┐
│ CODE NATIF CACHÉ (cache.bin) - x86 natif             │
│  - Code JIT compilé persisté sur disque              │
│  - Next boot : charge directement (~1s)              │
│  - Pas de warmup après première exécution            │
└──────────────────────────────────────────────────────┘
```

**Inspiration** :
- **PyPy** : Python écrit en Python, JIT lui-même
- **Truffle/Graal** : JVM JIT écrit en Java
- **LuaJIT** : JIT qui s'optimise lui-même
- **LLVM OrcJIT** : JIT qui compile du LLVM IR

---

## 📊 Approche Incrémentale (4 Phases)

### Phase 1 : AOT Simple (2-3 semaines) ✅ **PRIORITÉ IMMÉDIATE**

**Objectif** : Unikernel fonctionnel rapidement avec code natif

**Architecture** :
```
Build time:
  app.c → clang -emit-llvm → app.bc
  kernel_lib.c → clang -emit-llvm → kernel_lib.bc
  llvm-libc → llvmlibc.bc

  llvm-link app.bc kernel_lib.bc llvmlibc.bc → full.bc

  opt -O3 full.bc → optimized.bc

  llc -march=i686 optimized.bc → tinyllama.s

  clang tinyllama.s → tinyllama.elf

Boot time:
  → Exécution directe (code natif)
  → Profiling au runtime (rdtsc)
  → Pas de JIT, juste du natif
```

**Avantages** :
- ✅ Boot instantané (~0.5s)
- ✅ Performance maximale dès le départ
- ✅ Simple à implémenter
- ✅ Fonctionne de suite

**Limites** :
- ❌ Pas d'optimisation au runtime
- ❌ Pas de recompilation dynamique
- ❌ Taille fixe du binaire

**Deliverable** : `fluid_llama.img` (~100KB) bootable

---

### Phase 2 : Mini Interpreter (2-3 semaines)

**Objectif** : Prouver le concept de l'interpretation LLVM IR

**Architecture** :
```
Bootstrap natif (~10KB):
  llvm_bootstrap.c → clang → llvm_bootstrap.bin

  Contient:
  - Mini LLVM IR interpreter (read, decode, execute)
  - Load bitcode from disk
  - Execute LLVM IR instructions

LLVM IR application:
  tinyllama.c → clang -emit-llvm → tinyllama.bc

Boot:
  1. llvm_bootstrap.bin charge tinyllama.bc
  2. Interprète les instructions LLVM IR
  3. Exécution lente mais fonctionnelle
```

**Avantages** :
- ✅ Prouve que l'interpretation fonctionne
- ✅ Bootstrap minimal natif
- ✅ Flexibilité (pas besoin de recompiler)

**Limites** :
- ❌ Très lent (10-100x plus lent que natif)
- ❌ Pas encore d'optimisation

**Deliverable** : Interpreter LLVM IR fonctionnel

---

### Phase 3 : Meta-Circular Evaluation (4-6 semaines) 🔥 **VISION FINALE**

**Objectif** : Interpreter écrit en LLVM IR qui s'auto-interprète

**Architecture** :
```
llvm_bootstrap.bin (natif, 10KB):
  - Mini interpreter pour démarrer
  - Charge interpreter.bc
  - Exécute interpreter.bc (meta-circular!)

interpreter.bc (LLVM IR):
  - LLVM IR interpreter écrit en LLVM IR
  - S'auto-interprète via llvm_bootstrap
  - Profile ses propres hot paths
  - JIT compile ses hot paths en natif
  - Remplace llvm_bootstrap progressivement

jit_compiler.bc (LLVM IR):
  - Écrit en LLVM IR
  - Compile LLVM IR → code natif x86
  - Appelé par interpreter.bc
  - S'optimise lui-même aussi

profiler.bc (LLVM IR):
  - Écrit en LLVM IR
  - Profile TOUT (interpreter, JIT, app)
  - Décide quand recompiler
  - S'optimise lui-même

tinyllama.bc (LLVM IR):
  - Application finale
  - Exécutée par interpreter auto-optimisé
```

**Bootstrap Sequence** :
```
1. llvm_bootstrap.bin (natif) charge interpreter.bc
2. interpreter.bc s'exécute via llvm_bootstrap
3. interpreter.bc profile lui-même
4. interpreter.bc JIT compile ses hot paths
5. interpreter.bc devient progressivement natif
6. interpreter.bc charge profiler.bc
7. profiler.bc s'optimise
8. interpreter.bc charge jit_compiler.bc
9. jit_compiler.bc s'optimise
10. Tout le système est maintenant auto-optimisé
11. interpreter.bc charge tinyllama.bc
12. TinyLlama s'exécute avec tout le stack optimisé
```

**Cold Start (première fois)** :
- Temps : ~10-30s (tout interprété puis JIT compilé)
- Résultat : Code natif généré et caché

**Avantages** :
- ✅ **Auto-optimization totale**
- ✅ **Profile TOUT** (même l'interpreter)
- ✅ **Flexibilité maximale**
- ✅ **Philosophiquement parfait**

**Limites** :
- ❌ Bootstrap complex (paradoxe)
- ❌ Cold start lent (10-30s)
- ❌ Debugging très difficile

**Deliverable** : Meta-circular LLVM stack complet

---

### Phase 4 : Persistance & Cache (2-3 semaines)

**Objectif** : Éliminer le warmup après la première exécution

**Architecture** :
```
Première exécution (cold start):
  1. llvm_bootstrap.bin démarre
  2. Tout s'auto-optimise (10-30s)
  3. Code natif généré en mémoire
  4. Code natif sauvegardé sur disque: cache.bin

Deuxième exécution (warm start):
  1. llvm_bootstrap.bin charge cache.bin
  2. Exécution directe du code natif (~1s)
  3. Profiling continue
  4. Si nouveaux hot paths → recompile et met à jour cache

Structure du cache:
  cache.bin:
    - Header (version, checksum)
    - Interpreter natif compilé
    - JIT compiler natif compilé
    - Profiler natif compilé
    - TinyLlama partiellement compilé (hot paths)
    - Profiling data
```

**Avantages** :
- ✅ **Warmup 1 seule fois**
- ✅ **Boot rapide après** (~1s)
- ✅ **Continue à s'améliorer**
- ✅ **Mémoire persistent**

**Limites** :
- ❌ Complexité du cache management
- ❌ Invalidation du cache si changements
- ❌ Gestion de versions

**Deliverable** : Système complet avec persistance

---

## 🔧 Build Pipeline Détaillé

### Phase 1 : AOT Pipeline (Immédiat)

```bash
# 1. Compile tout en LLVM bitcode
clang-18 -m32 -ffreestanding -emit-llvm -c tinyllama/main.c -o build/main.bc
clang-18 -m32 -ffreestanding -emit-llvm -c kernel_lib/io/vga.c -o build/vga.bc
# ... pour tous les fichiers ...

# 2. Link tout le bitcode
llvm-link-18 build/*.bc -o build/tinyllama_full.bc

# 3. Optimize avec LTO
opt-18 -O3 -march=i686 build/tinyllama_full.bc -o build/tinyllama_opt.bc

# 4. Generate assembly
llc-18 -O3 -march=i686 build/tinyllama_opt.bc -o build/tinyllama.s

# 5. Assemble & link
clang-18 -m32 -nostdlib build/tinyllama.s boot/entry.o -T linker.ld -o build/tinyllama.elf

# 6. Create binary
objcopy -O binary build/tinyllama.elf build/tinyllama.bin

# 7. Create bootable image
cat boot/stage1.bin boot/stage2.bin build/tinyllama.bin > fluid_llama.img
```

### Phase 2 : Interpreter Bootstrap

```c
// llvm_bootstrap.c (compilé en natif)
typedef struct {
    uint8_t opcode;
    uint32_t operands[3];
} LLVMInstruction;

void llvm_execute(LLVMInstruction* inst) {
    switch (inst->opcode) {
        case LLVM_ADD: /* ... */ break;
        case LLVM_LOAD: /* ... */ break;
        case LLVM_STORE: /* ... */ break;
        case LLVM_CALL: /* ... */ break;
        // ... 50-100 opcodes LLVM IR ...
    }
}

void bootstrap_main() {
    // 1. Load interpreter.bc from disk
    LLVMModule* interp = load_bitcode("interpreter.bc");

    // 2. Execute interpreter.bc
    // (interpreter va s'auto-interpréter!)
    llvm_execute_module(interp);
}
```

### Phase 3 : Meta-Circular Interpreter

```llvm
; interpreter.bc (écrit en LLVM IR)

define void @interpreter_main() {
entry:
  ; 1. Profile moi-même
  call void @profiler_begin(i8* getelementptr([11 x i8], [11 x i8]* @.str_interp, i32 0, i32 0))

  ; 2. Charge le module à interpréter
  %module = call %LLVMModule* @load_bitcode(i8* %app_path)

  ; 3. Pour chaque fonction du module
  br label %interpret_loop

interpret_loop:
  %inst = call %LLVMInstruction* @fetch_instruction(%module)

  ; 4. Exécute l'instruction
  call void @execute_instruction(%inst)

  ; 5. Check si hot path
  %is_hot = call i1 @profiler_is_hot(i8* %func_name)
  br i1 %is_hot, label %jit_compile, label %continue

jit_compile:
  ; 6. JIT compile cette fonction
  call void @jit_compile_function(%func_name)
  br label %continue

continue:
  call void @profiler_end(i8* getelementptr([11 x i8], [11 x i8]* @.str_interp, i32 0, i32 0))
  br label %interpret_loop
}
```

---

## 📈 Performance Comparison

| Phase | Cold Start | Warm Start | Peak Perf | Complexity |
|-------|-----------|------------|-----------|------------|
| **1. AOT** | 0.5s | 0.5s | 100% | Low |
| **2. Interpreter** | 5s | 5s | 5-10% | Medium |
| **3. Meta-Circular** | 10-30s | 1s | 95-100% | **High** |
| **4. + Persistance** | 10-30s (once) | 0.5s | 95-100% | **Very High** |

---

## 🎯 Recommandation

### Court Terme (Sessions 18-20) : **Phase 1 AOT**
- Simple, rapide, fonctionnel
- Permet de valider l'architecture unikernel
- Base solide pour les phases suivantes

### Moyen Terme (Sessions 21-25) : **Phase 2 Interpreter**
- Prouve le concept
- Permet d'expérimenter avec LLVM IR
- Prépare Phase 3

### Long Terme (Sessions 26-35) : **Phase 3 Meta-Circular**
- Vision finale
- Auto-optimization totale
- Innovation majeure

### Très Long Terme (Sessions 36-40) : **Phase 4 Persistance**
- Élimine le warmup
- Performance maximale
- Système complet

---

## 🔬 Références Techniques

### PyPy (Python in Python)
- Interpreter Python écrit en RPython
- JIT compile lui-même
- 5-10x plus rapide que CPython

### Truffle/Graal (Java JIT in Java)
- JVM JIT écrit en Java
- Compile Java → code natif
- Self-optimizing

### LuaJIT (Lua JIT)
- Interpreter + JIT en C
- Trace compilation
- Ultra-rapide (souvent plus rapide que code natif!)

### LLVM OrcJIT
- JIT compilation de LLVM IR
- Utilisé par Swift, Julia, etc.
- Reference implementation pour notre Phase 3

---

**Auteur** : BareFlow Team (Session 17)
**Status** : Phase 1 en cours, Phases 2-4 planifiées
**Vision** : Meta-circular self-optimizing LLVM unikernel
