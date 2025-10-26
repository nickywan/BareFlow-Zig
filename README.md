# BareFlow - Hybrid Self-Optimizing Unikernel

**"Grow to Shrink" - 68000-style Self-Tuning Program** (x86-64)

> "Le kernel n'est plus qu'une bibliothèque d'accès au processeur, ce n'est pas un kernel
> (un cœur) mais juste un profiler qui aide le programme à fonctionner du mieux possible."

---

## 🎯 Vision: Convergence Progressive

BareFlow est un **programme auto-suffisant** qui s'inspire des programmes 68000-style:

```
Boot 1:    [60MB] Full LLVM + app en IR → Interprété (lent mais profile TOUT)
Boot 100:  [30MB] Hot paths JIT O0→O3   → 10× plus rapide
Boot 500:  [10MB] Dead code éliminé     → Relink optimisé
Boot Final:[2-5MB] Pure native, no LLVM → Appliance hardware-optimized
```

### Philosophie "Grow to Shrink"

**On s'en fiche de la taille initiale!** Le programme:
1. **Embarque tout** (60MB: LLVM complet + LLVM-libc + app en IR)
2. **S'auto-profile** (track ALL functions: app + LLVM + libc)
3. **JIT optimise** (hot paths: O0 → O1 → O2 → O3 + specialization)
4. **Élimine le mort** (40% du code jamais utilisé)
5. **Converge** (vers 2-5MB optimisé pour CE hardware + CE workload)
6. **Persiste** (snapshot → appliance bootable)

**Inspirations**: PyPy warmup snapshots + LuaJIT tiered compilation + V8 PGO + programmes 68000 self-contained

**Pas de kernel traditionnel**. Pas de séparation kernel/user. Pas de syscalls.
Une application qui s'auto-profile, se recompile à chaud et s'optimise en temps réel.

### État Actuel (Branch: feat/true-jit-unikernel)

✅ **Phase 1-2 COMPLÈTE**: Unikernel de base + Profiling
- Unikernel fonctionnel (28KB: 13KB app + 15KB runtime)
- Profiling cycle-accurate (rdtsc)
- Métriques AOT baseline documentées

✅ **Phase 3 COMPLÈTE**: Hybrid Self-Optimizing Runtime (Userspace Validation)
- Phase 3.1 ✅: LLVM JIT verification (LLVM 18.1.8)
- Phase 3.2 ✅: Static linking research (hybrid approach confirmed)
- Phase 3.3 ✅: LLVM Interpreter validation (383× speedup Interpreter→JIT)
- Phase 3.4 ✅: Tiered JIT (O0→O3 automatic, 1.17× vs AOT)
- Phase 3.5 ✅: Dead code analysis (99.83% unused in LLVM)
- Phase 3.6 ✅: Native export (118 MB → 20 KB, 6000× reduction!)

✅ **Phase 4 COMPLÈTE**: Bare-Metal JIT Integration (x86-64)
- Session 23 ✅: LLVM 18 validation (545 MB full installation)
- Session 24 ✅: C++ runtime (12 KB bare-metal C++ support)
- Session 25 ✅: Enhanced allocator (200 MB heap, free-list)
- Session 26 ✅: Bare-metal integration (kernel_lib_llvm.a 23 KB)
- Session 27 ✅: Strategy & analysis (32-bit vs 64-bit)
- Session 28 ✅: Enhanced LLVM tests (1.7× speedup O0→O1)
- **DECISION**: Migration to x86-64 (better JIT, native LLVM)
- Session 29 ✅: QEMU x86-64 boot successful (Multiboot2)
- Session 30 ✅: Phase 4 finalization & documentation
- **Result**: 64-bit runtime (kernel_lib_llvm.a 29 KB) + QEMU validation

🚀 **Phase 5 EN COURS**: TinyLlama Model Integration
- Session 31 ✅: malloc Investigation (paging + bump allocator)
- **Result**: Paging working ✅, bump allocator working ✅
- **Finding**: Problem isolated to free-list in malloc_llvm.c
- Session 32: Continue with bump allocator or fix malloc_llvm.c
- Session 33: Model loading (TinyLlama weights ~60MB)
- Session 34-36: Inference optimization (matrix multiply, vectorization)
- Session 37-39: Self-optimization (JIT hot layers, convergence)

---

## 🏗️ Architecture

### Phase 1-2 (AOT Baseline) ✅ ACTUELLE
```
┌─────────────────────────────────────────────────────┐
│  tinyllama.img (10MB bootable)                      │
│  ┌─────────────────────────────────────────────┐   │
│  │ TinyLlama Application (13KB)                │   │
│  │  - Demo functions (fibonacci, sum, primes)  │   │
│  │  - Self-profiling (jit_profile_*)           │   │
│  │  - AOT-compiled (-O2)                       │   │
│  └─────────────────────────────────────────────┘   │
│             ↓ direct calls (24 cycles/call)        │
│  ┌─────────────────────────────────────────────┐   │
│  │ Runtime Library (kernel_lib.a 15KB)         │   │
│  │  - I/O: VGA, serial, keyboard               │   │
│  │  - Memory: malloc, memcpy, string           │   │
│  │  - CPU: rdtsc, cpuid, PIC, IDT              │   │
│  │  - JIT: Profiling (cycle-accurate)          │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

### Phase 3 (Hybrid Self-Optimizing) ⚠️ EN COURS
```
┌─────────────────────────────────────────────────────┐
│  Boot 1: [60MB] Full LLVM + LLVM-libc (static)      │
│  ┌─────────────────────────────────────────────┐   │
│  │  - LLVM Interpreter (execute IR directly)   │   │
│  │  - LLVM OrcJIT (tiered compilation)         │   │
│  │  - LLVM-libc (pure C, JIT-optimizable)      │   │
│  │  - Profiler (track ALL calls + cycles)      │   │
│  │  - Coverage (detect dead code)              │   │
│  └─────────────────────────────────────────────┘   │
│                                                     │
│  Boot 100: [30MB] JIT O0→O3 on hot paths            │
│  Boot 500: [10MB] Dead code eliminated              │
│  Boot Final: [2-5MB] Pure native, LLVM removed      │
└─────────────────────────────────────────────────────┘
```

### Why JIT for TinyLlama?

**TinyLlama = language model** → Perfect use case for JIT:

1. **Matrix multiply specialization**: JIT observes actual matrix sizes (always 512×512)
   - AOT generic: ~1000ms (handles any size)
   - JIT specialized: ~50ms (**20× speedup!**)

2. **Hardware vectorization**: JIT detects real CPU (AVX2/AVX512)
   - AOT conservative: SSE2 (runs everywhere)
   - JIT aggressive: AVX512 if available (**3× speedup**)

3. **LLVM-libc optimization**: `memcpy(dst, src, 512)` always same size
   - AOT generic: handles any size, alignment
   - JIT: AVX2 unrolled 8×64 bytes (**10× speedup**)

**Total expected gain**: **2-5× on hot paths** vs generic AOT O3

---

## 📦 Structure du Projet

```
BareFlow-LLVM/
├── boot/
│   ├── stage1.asm          # MBR bootloader
│   └── stage2.asm          # Extended bootloader
├── kernel_lib/             # Runtime library (C + C++)
│   ├── io/                 # I/O drivers
│   │   ├── vga.{h,c}       # VGA text mode
│   │   ├── serial.{h,c}    # Serial port
│   │   └── keyboard.{h,c}  # PS/2 keyboard
│   ├── memory/             # Memory management
│   │   ├── malloc.{h,c}    # Simple bump allocator (256 KB)
│   │   ├── malloc_llvm.c   # ← NEW: Free-list allocator (200 MB)
│   │   └── string.{h,c}    # String functions
│   ├── cpp_runtime/        # ← NEW: Bare-metal C++ runtime (12 KB)
│   │   ├── new.cpp         # operator new/delete
│   │   ├── exception.cpp   # Exception stubs (-fno-exceptions)
│   │   ├── atexit.cpp      # Static initialization guards
│   │   ├── syscall_stubs.cpp # System call stubs for LLVM
│   │   └── Makefile        # Build cpp_runtime.a
│   ├── cpu/                # CPU features
│   │   └── features.{h,c}  # rdtsc, cpuid, PIC, IDT
│   ├── jit/                # JIT profiling
│   │   └── profile.{h,c}   # Cycle-accurate profiling
│   ├── runtime.h           # Public API (I/O + Memory + CPU)
│   ├── jit_runtime.h       # Public API (JIT)
│   └── Makefile            # Build kernel_lib.a (15 KB)
├── tinyllama/              # Unikernel application
│   ├── entry.asm           # Entry point with FLUD signature
│   ├── main.c              # Demo profiling (fib, sum, primes)
│   ├── linker.ld           # Linker script (0x10000)
│   └── Makefile            # Build tinyllama_bare.bin + .img
├── tests/                  # Validation tests
│   ├── phase3/             # Phase 3 userspace validation (17 tests)
│   │   ├── test_llvm_interpreter.cpp   # 383× speedup validation
│   │   ├── test_tiered_jit.cpp         # O0→O3 tiered compilation
│   │   └── test_native_export.cpp      # 6000× size reduction
│   └── phase4/             # ← NEW: Phase 4 bare-metal prep
│       ├── test_cpp_runtime.cpp        # C++ runtime validation
│       ├── test_malloc_llvm.cpp        # Free-list allocator tests
│       └── test_llvm_init.cpp          # LLVM initialization test
├── docs/                   # Documentation
│   ├── phase3/             # Phase 3 results
│   │   ├── PHASE3_3_RESULTS.md         # 399× speedup proof
│   │   ├── PHASE3_4_TIERED_JIT.md      # Tiered compilation
│   │   ├── PHASE3_5_DCE_RESULTS.md     # 99.83% dead code
│   │   └── PHASE3_6_NATIVE_EXPORT.md   # 6000× reduction
│   └── phase4/             # ← NEW: Phase 4 documentation
│       ├── PHASE4_BAREMETAL_REQUIREMENTS.md
│       ├── SESSION_23_SUMMARY.md       # LLVM validation
│       ├── SESSION_24_SUMMARY.md       # C++ runtime
│       └── SESSION_25_SUMMARY.md       # (en cours)
├── archive/                # Archived code
│   └── kernel/             # Old monolithic kernel
└── README.md               # This file
```

---

## 🚀 Quick Start (Phase 1-2 - AOT Baseline)

### Build Everything

```bash
# 1. Build runtime library
cd kernel_lib
make
# → kernel_lib.a (15KB)

# 2. Build unikernel application
cd ../tinyllama
make rebuild
# → tinyllama_bare.bin (13KB)
# → tinyllama.img (10MB bootable)
```

### Run in QEMU

```bash
cd tinyllama
make run
# Ou pour voir output serial:
qemu-system-i386 -drive file=tinyllama.img,format=raw -serial stdio
```

### Expected Output

```
[tinyllama] TinyLlama Unikernel v0.1 - Self-Profiling Demo
[tinyllama] Initializing JIT profiler...

[BENCH] 10000 calls: 241131 cycles (24 cycles/call)

fibonacci: calls=10, avg=32638, min=6235, max=267864
sum_to_n: calls=100, avg=549, min=174, max=19960
count_primes: calls=5, avg=50451, min=10088, max=209608

=== PERFORMANCE TIMING ===
[TIMING] Initialization:  3146383 cycles
[TIMING] Test 1 (Fib):    8271883 cycles
[TIMING] Test 2 (Sum):    1527162 cycles
[TIMING] Test 3 (Primes): 1648157 cycles
[TIMING] Total execution: 16353646 cycles
```

---

## 📊 AOT Baseline Metrics

Ces métriques servent de **référence** pour les comparaisons JIT futures:

- **Binary size**: 28KB (92% réduction vs 321KB monolithic)
- **Function call overhead**: 24 cycles/call (inline, direct)
- **Boot time**: ~3.1M cycles initialization
- **Total execution**: ~16.4M cycles (3 tests)

---

## 🔧 Next: Build Bootable Image (Legacy)

```bash
objcopy -O binary tinyllama_bare.elf tinyllama_bare.bin
cat boot/stage1.bin boot/stage2.bin tinyllama_bare.bin > fluid_llama.img
```

### Run in QEMU

```bash
qemu-system-i386 -drive file=fluid_llama.img,format=raw -serial stdio
```

---

## 💻 Example Usage

```c
// tinyllama/main.c
#include "runtime.h"      // VGA, serial, malloc
#include "jit_runtime.h"  // JIT profiling & optimization

void inference_loop(void) {
    jit_profile_begin("inference");

    // ... TinyLlama model inference ...

    jit_profile_end("inference");

    // Self-optimization: Application decides when to optimize
    static int call_count = 0;
    if (++call_count % 1000 == 0) {
        jit_optimize_hot_functions();
    }
}

void main(void) {
    vga_init();
    serial_init();

    vga_puts("TinyLlama Unikernel v1.0\n");

    while (1) {
        inference_loop();
    }
}
```

---

## 📊 Performance Benefits

| Metric | Monolithic Kernel | Unikernel | Improvement |
|--------|------------------|-----------|-------------|
| **Binary Size** | 346KB | ~100KB | **70% smaller** |
| **Function Call** | ~10-20 cycles (syscall) | ~1-2 cycles (direct) | **10x faster** |
| **Boot Time** | ~3-5s | ~0.5-1s | **5x faster** |
| **Memory Usage** | 16MB | 256KB | **98% less** |

---

## 🎓 API Reference

### Runtime API (runtime.h)

```c
// I/O
void vga_init(void);
void vga_puts(const char* str);
void serial_init(void);
void serial_puts(const char* str);
int keyboard_getchar(void);

// Memory
void* malloc(size_t size);
void free(void* ptr);
void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* s, int c, size_t n);

// CPU
uint64_t cpu_rdtsc(void);
void cpu_cpuid(uint32_t leaf, uint32_t* eax, ...);
```

### JIT Runtime API (jit_runtime.h)

```c
// Profiling
void jit_profile_begin(const char* func_name);
void jit_profile_end(const char* func_name);
uint64_t jit_get_call_count(const char* func_name);
uint64_t jit_get_avg_cycles(const char* func_name);

// Optimization
void jit_optimize_hot_functions(void);
void jit_set_threshold(int warm, int hot, int very_hot);
int jit_get_optimization_level(const char* func_name);
```

---

## 🔧 Build Requirements

- **LLVM 18** : clang-18, ld.lld-18, llvm-ar-18
- **NASM** : For bootloader assembly
- **QEMU** : qemu-system-i386 for testing
- **Make** : GNU Make

```bash
# Ubuntu/Debian
sudo apt install clang-18 llvm-18 nasm qemu-system-x86
```

---

## 📖 Documentation

- **[ARCHITECTURE_UNIKERNEL.md](ARCHITECTURE_UNIKERNEL.md)** : Architecture détaillée
- **[ROADMAP.md](ROADMAP.md)** : Phase 6 - Unikernel Refactor
- **[NEXT_SESSION_UNIKERNEL.md](NEXT_SESSION_UNIKERNEL.md)** : Guide pour Session 18
- **[CLAUDE_CONTEXT.md](CLAUDE_CONTEXT.md)** : Session 17 - Décision architecturale

---

## 🎯 Roadmap

### ✅ Session 17 (COMPLETE)
- [x] Décision architecturale : Option 2 (unikernel)
- [x] Design kernel_lib.a structure
- [x] Documentation complète
- [x] Mise à jour ROADMAP + CONTEXT

### 🔥 Session 18 (IN PROGRESS)
- [ ] Extraction kernel_lib.a
- [ ] API publique (runtime.h, jit_runtime.h)
- [ ] Build system (Makefile.lib)
- [ ] Validation

### 🚀 Session 19 (PLANNED)
- [ ] Créer tinyllama/ stub
- [ ] Linker avec kernel_lib.a
- [ ] Générer tinyllama_bare.elf
- [ ] Boot test

### 🎓 Sessions 20-22 (FUTURE)
- [ ] Intégration TinyLlama complet
- [ ] Benchmarking vs ancien kernel
- [ ] Optimisations finales

---

## 🤝 Philosophy

Cette architecture s'inspire de :
- **LuaJIT** : Auto-optimization
- **V8** : Tiered compilation
- **MirageOS** : Unikernel minimalism
- **Unikraft** : Composable libraries

**Principe fondamental** : Le runtime est au **service** de l'application, pas l'inverse.

---

## 📝 License

MIT License - See LICENSE file

---

**Auteur** : BareFlow Team
**Status** : 🚧 **In Active Development** - Phase 6 (Unikernel Refactor)
**Version** : 2.0.0-alpha (Unikernel Architecture)
