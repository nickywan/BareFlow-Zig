# BareFlow - Architecture Unikernel (Option 2)

**Date**: 2025-10-25 (Session 17)
**Decision**: Programme Auto-Optimisant vs Kernel + Modules Chargés

---

## 🎯 Décision Architecturale

Après avoir atteint 346KB de kernel monolithique avec 7 modules LLVM embarqués (28 binaires),
nous avons décidé d'adopter une **architecture unikernel auto-optimisante** plutôt qu'un
kernel traditionnel avec chargement de modules.

> "Le kernel n'est plus qu'une bibliothèque d'accès au processeur, ce n'est pas un kernel
> (un cœur) mais juste un profiler qui aide le programme à fonctionner du mieux possible."

---

## 📊 Comparaison des Architectures

### Option 1 : Kernel + Modules Chargés (REJETÉE)

```
┌─────────────────────────────────────┐
│  Kernel (bootloader + OS)           │
│  - VGA, serial, memory, CPU         │
│  - ELF loader                       │
│  - JIT profiler & optimizer         │
│  - Module manager                   │
│  Size: ~100KB                       │
└─────────────────────────────────────┘
              ↓ load()
┌─────────────────────────────────────┐
│  Application (TinyLlama)            │
│  - Model inference                  │
│  - Appels kernel APIs (syscalls)    │
│  Size: ~60-80KB                     │
└─────────────────────────────────────┘

Total: ~160-180KB + overhead
```

**Problèmes** :
- ❌ Overhead : Context switch kernel/user
- ❌ Complexité : ELF loader, ABI, syscalls
- ❌ Deux binaries : kernel.bin + app.elf
- ❌ Kernel grossit : Infrastructure JIT complète
- ❌ Séparation artificielle : Ring 0/Ring 3 inutile

### Option 2 : Programme Auto-Optimisant (RETENUE ✅)

```
┌─────────────────────────────────────────────────────┐
│  Single Binary: tinyllama_bare.elf                  │
│  ┌─────────────────────────────────────────────┐   │
│  │ Application (TinyLlama)                     │   │
│  │  - Model loading & inference                │   │
│  │  - Self-profiling (jit_profile_*)           │   │
│  │  - Self-optimization (jit_optimize_*)       │   │
│  │  Size: ~60-80KB                             │   │
│  └─────────────────────────────────────────────┘   │
│             ↓ appels directs (linked statically)   │
│  ┌─────────────────────────────────────────────┐   │
│  │ Runtime Library (kernel_lib.a)              │   │
│  │  - I/O: vga_*, serial_*, keyboard_*         │   │
│  │  - Memory: malloc, free, memcpy             │   │
│  │  - CPU: rdtsc, cpuid, features              │   │
│  │  - JIT: profile, optimize, hot-path detect  │   │
│  │  Size: ~20-30KB                             │   │
│  └─────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
         ↓ clang -nostdlib -lkernel_lib
    fluid_llama.img (~80-100KB bootable)
```

**Avantages** :
- ✅ **Zero overhead** : Appels directs, pas de syscalls
- ✅ **Simplicité maximale** : Un seul binaire
- ✅ **Self-optimization** : Le programme s'optimise lui-même
- ✅ **Performance** : Tout en Ring 0, contrôle total
- ✅ **Philosophie correcte** : Runtime au service de l'app, pas l'inverse
- ✅ **Taille réduite** : ~100KB vs 346KB (65-70% réduction)

---

## 🏗️ Structure du Code

### kernel_lib/ (Runtime Library)

```
kernel_lib/
├── io/
│   ├── vga.h              # VGA API publique
│   ├── vga.c              # VGA text mode 80x25
│   ├── serial.h           # Serial API publique
│   ├── serial.c           # COM1 serial port
│   ├── keyboard.h         # Keyboard API publique
│   └── keyboard.c         # PS/2 keyboard driver
├── memory/
│   ├── malloc.h           # Memory API publique
│   ├── malloc.c           # Simple bump allocator
│   ├── string.h           # String API publique
│   └── string.c           # memcpy, memset, strlen, etc.
├── cpu/
│   ├── features.h         # CPU API publique
│   ├── features.c         # cpuid, rdtsc, cpu_info
│   └── interrupts.c       # IDT setup (si nécessaire)
├── jit/
│   ├── profile.h          # Profiling API publique
│   ├── profile.c          # jit_profile_begin/end
│   ├── optimize.h         # Optimization API publique
│   ├── optimize.c         # jit_optimize_hot_functions
│   └── adaptive.c         # Adaptive optimization logic
├── runtime.h              # API publique unifiée (I/O + Memory + CPU)
├── jit_runtime.h          # API publique JIT (Profile + Optimize)
└── Makefile.lib           # Build kernel_lib.a
```

**API Publique** (runtime.h) :
```c
// I/O
void vga_init(void);
void vga_putchar(char c);
void vga_puts(const char* str);
void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char* str);
int keyboard_getchar(void);

// Memory
void* malloc(size_t size);
void free(void* ptr);
void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
size_t strlen(const char* s);

// CPU
uint64_t cpu_rdtsc(void);
void cpu_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);
```

**API Publique** (jit_runtime.h) :
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

### tinyllama/ (Application Auto-Optimisante)

```
tinyllama/
├── main.c                 # Entry point + self-optimization
├── inference.c            # Model inference loop
├── profiling.c            # Self-profiling instrumentation
├── optimization.c         # Self-optimization decisions
├── model.h                # TinyLlama model structures
└── Makefile.tinyllama     # Build tinyllama_bare.elf
```

**Exemple** (main.c) :
```c
#include "runtime.h"      // VGA, serial, malloc, etc.
#include "jit_runtime.h"  // JIT profiling & optimization

void inference_loop(void) {
    jit_profile_begin("inference");

    // ... TinyLlama inference code ...
    // Matrix multiplications, attention, etc.

    jit_profile_end("inference");

    // Self-optimization: L'application décide quand optimiser
    static int call_count = 0;
    if (++call_count % 1000 == 0) {
        // Après 1000 inférences, réoptimiser les hot paths
        jit_optimize_hot_functions();

        serial_puts("Self-optimization complete. Stats:\n");
        serial_puts("  inference: ");
        char buf[32];
        sprintf(buf, "%llu calls, %llu avg cycles\n",
                jit_get_call_count("inference"),
                jit_get_avg_cycles("inference"));
        serial_puts(buf);
    }
}

void main(void) {
    // Initialize runtime
    vga_init();
    serial_init();

    vga_puts("TinyLlama Self-Optimizing Unikernel v1.0\n");
    serial_puts("\n=== TinyLlama Boot ===\n");

    // Load model (placeholder)
    serial_puts("Loading model...\n");
    // ... load model weights ...

    // Main inference loop
    serial_puts("Starting inference loop...\n");
    while (1) {
        inference_loop();
    }
}
```

---

## 🔧 Build System

### Makefile.lib (Build Runtime Library)

```makefile
# Compile kernel_lib.a
CC = clang-18
AR = llvm-ar-18
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra

SRCS = io/vga.c io/serial.c io/keyboard.c \
       memory/malloc.c memory/string.c \
       cpu/features.c cpu/interrupts.c \
       jit/profile.c jit/optimize.c jit/adaptive.c

OBJS = $(SRCS:.c=.o)

kernel_lib.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) kernel_lib.a
```

### Makefile.tinyllama (Build Application)

```makefile
# Compile tinyllama_bare.elf
CC = clang-18
LD = ld.lld-18
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

SRCS = main.c inference.c profiling.c optimization.c
OBJS = $(SRCS:.c=.o)

tinyllama_bare.elf: $(OBJS) ../kernel_lib/kernel_lib.a
	$(LD) $(LDFLAGS) -o $@ boot/entry.o $(OBJS) -L../kernel_lib -lkernel_lib

%.o: %.c
	$(CC) $(CFLAGS) -I../kernel_lib -c $< -o $@

clean:
	rm -f $(OBJS) tinyllama_bare.elf
```

### Build Workflow

```bash
# 1. Build runtime library
cd kernel_lib
make -f Makefile.lib
# → kernel_lib.a (~20-30KB)

# 2. Build application
cd ../tinyllama
make -f Makefile.tinyllama
# → tinyllama_bare.elf (~80-100KB)

# 3. Create bootable image
objcopy -O binary tinyllama_bare.elf tinyllama_bare.bin
cat ../boot/stage1.bin ../boot/stage2.bin tinyllama_bare.bin > fluid_llama.img

# 4. Boot in QEMU
qemu-system-i386 -drive file=fluid_llama.img,format=raw -serial stdio
```

---

## 📈 Métriques de Validation

### Binary Size

| Architecture | Kernel | App | Total | Réduction |
|--------------|--------|-----|-------|-----------|
| Monolithique (actuel) | 346KB | - | 346KB | - |
| Unikernel (cible) | 20-30KB (lib) | 60-80KB | 80-100KB | **~70%** |

### Performance

| Métrique | Monolithique | Unikernel | Amélioration |
|----------|-------------|-----------|--------------|
| Function call overhead | ~10-20 cycles (syscall) | ~1-2 cycles (direct) | **10x** |
| Boot time | ~3-5s (init tout) | ~0.5-1s (direct app) | **5x** |
| Memory usage | 16MB heap + pools | 256KB heap | **98%** |

### Complexité

| Aspect | Monolithique | Unikernel |
|--------|-------------|-----------|
| Fichiers sources | ~50 files | ~20 files |
| Lignes de code | ~8000 LOC | ~3000 LOC |
| Build time | ~30s | ~5s |
| Debugging complexity | High (kernel + modules) | Low (single binary) |

---

## 🚀 Roadmap d'Implémentation

### Semaine 1 : Extraction kernel_lib.a
- [x] Design architecture (Session 17)
- [ ] Créer structure kernel_lib/
- [ ] Extraire VGA, serial, keyboard
- [ ] Extraire malloc, string functions
- [ ] Extraire CPU features (rdtsc, cpuid)
- [ ] Créer Makefile.lib
- [ ] Valider compilation kernel_lib.a

### Semaine 2 : Application Stub
- [ ] Créer structure tinyllama/
- [ ] Implémenter main.c basique
- [ ] Ajouter self-profiling
- [ ] Ajouter self-optimization logic
- [ ] Créer Makefile.tinyllama
- [ ] Tester linking avec kernel_lib.a

### Semaine 3 : Bootable Image
- [ ] Mettre à jour linker script
- [ ] Générer tinyllama_bare.bin
- [ ] Créer fluid_llama.img
- [ ] Tester boot dans QEMU
- [ ] Valider VGA + serial output

### Semaine 4 : Validation
- [ ] Benchmarker architecture actuelle
- [ ] Benchmarker nouvelle architecture
- [ ] Comparer métriques
- [ ] Documenter résultats
- [ ] Mettre à jour documentation

### Semaines 5-6 : Migration TinyLlama
- [ ] Porter inference code
- [ ] Intégrer model loading
- [ ] Valider correctness
- [ ] Optimiser hot paths
- [ ] Tests end-to-end

---

## 🎓 Références & Inspiration

Cette architecture s'inspire de :

1. **LuaJIT** (Mike Pall) : JIT auto-optimisant avec profiling inline
2. **V8** (Google) : Tiered compilation (Ignition → TurboFan)
3. **MirageOS** : Unikernel OCaml avec runtime minimal
4. **Unikraft** : Unikernel modulaire avec bibliothèques composables
5. **IncludeOS** : C++ unikernel avec zero overhead

**Philosophie commune** : Le runtime est au **service** de l'application, pas l'inverse.

---

**Auteur** : Claude (Session 17)
**Validé par** : Architecture review (utilisateur + Claude)
**Status** : ✅ **APPROUVÉ** - Phase 6 ROADMAP
