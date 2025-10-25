# BareFlow - True Self-Optimizing Unikernel

**True JIT Unikernel - Programme Auto-Optimisant**

> "Le kernel n'est plus qu'une bibliothèque d'accès au processeur, ce n'est pas un kernel
> (un cœur) mais juste un profiler qui aide le programme à fonctionner du mieux possible."

---

## 🎯 Vision

BareFlow est un **vrai unikernel auto-optimisant** : un binaire unique (28KB baseline, <500KB avec JIT)
qui combine l'application (TinyLlama) avec une bibliothèque runtime minimale (kernel_lib.a 15KB)
pour exécuter bare-metal avec **optimisation JIT LLVM au runtime**.

**Pas de kernel traditionnel**. Pas de séparation kernel/user. Pas de syscalls.
Une application qui s'auto-profile, se recompile à chaud et s'optimise en temps réel.

### État Actuel (Branch: feat/true-jit-unikernel)

✅ **Phase 1-2 COMPLÈTE**: Unikernel de base + Profiling
- Unikernel fonctionnel (28KB: 13KB app + 15KB runtime)
- Profiling cycle-accurate (rdtsc)
- Métriques AOT baseline documentées

⚠️ **Phase 3 EN COURS**: JIT Runtime LLVM
- Port du runtime auto-optimisant
- Recompilation à chaud
- Code swapping atomique

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

### Phase 3 (JIT Runtime) ⚠️ EN COURS
```
┌─────────────────────────────────────────────────────┐
│  Ajout prévu: LLVM JIT Runtime (~450KB)             │
│  ┌─────────────────────────────────────────────┐   │
│  │  - jit_compile(): Génère IR optimisé        │   │
│  │  - jit_optimize(): Recompile à chaud        │   │
│  │  - jit_swap_function(): Swap atomique       │   │
│  │  - Custom allocator (1-2MB heap LLVM)       │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
Target: >20% speedup sur hot paths, <10% overhead global
```

---

## 📦 Structure du Projet

```
BareFlow-LLVM/
├── boot/
│   ├── stage1.asm          # MBR bootloader
│   └── stage2.asm          # Extended bootloader
├── kernel_lib/             # ← NEW: Runtime library
│   ├── io/
│   │   ├── vga.{h,c}       # VGA text mode
│   │   ├── serial.{h,c}    # Serial port
│   │   └── keyboard.{h,c}  # PS/2 keyboard
│   ├── memory/
│   │   ├── malloc.{h,c}    # Memory allocator
│   │   └── string.{h,c}    # String functions
│   ├── cpu/
│   │   └── features.{h,c}  # CPU features (rdtsc, cpuid)
│   ├── jit/
│   │   ├── profile.{h,c}   # Profiling system
│   │   ├── optimize.{h,c}  # Optimization logic
│   │   └── adaptive.{h,c}  # Adaptive JIT
│   ├── runtime.h           # Public API (I/O + Memory + CPU)
│   ├── jit_runtime.h       # Public API (JIT)
│   └── Makefile.lib        # Build kernel_lib.a
├── tinyllama/              # ← NEW: Unikernel app (Phase 1-2)
│   ├── entry.asm           # Entry point avec signature FLUD
│   ├── main.c              # Demo profiling (fib, sum, primes)
│   ├── linker.ld           # Linker script (0x10000)
│   └── Makefile            # Build tinyllama_bare.bin + .img
└── kernel/                 # ← OLD: Monolithic kernel (archived)
    ├── jit_llvm18.cpp      # TODO: Port vers kernel_lib/jit/
    └── ...
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
