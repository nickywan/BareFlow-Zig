# Code Reuse Mapping - Session 17

**Objectif**: Identifier le code réutilisable de l'implémentation actuelle pour kernel_lib.a

---

## ✅ RÉUTILISABLE DIRECTEMENT (80% du code)

### I/O Drivers → kernel_lib/io/

| Fichier Actuel | Destination | Action | Taux Réutilisation |
|---------------|-------------|--------|-------------------|
| `kernel/vga.{h,c}` | `kernel_lib/io/vga.{h,c}` | **Copie directe** | 100% |
| `kernel/keyboard.h` | `kernel_lib/io/keyboard.{h,c}` | Compléter avec implémentation | 95% |
| `kernel/fat16.{h,c}` | `kernel_lib/io/fat16.{h,c}` | Copie directe (optionnel) | 100% |
| `kernel/pic.{h,c}` | `kernel_lib/cpu/pic.{h,c}` | Copie directe | 100% |
| `kernel/idt.{h,c}` | `kernel_lib/cpu/idt.{h,c}` | Copie directe | 100% |

**Serial**: Extraction depuis kernel.c (fonctions serial_*)

### Memory Management → kernel_lib/memory/

| Fonctions (kernel/stdlib.c) | Destination | Action |
|----------------------------|-------------|--------|
| `malloc`, `free`, `calloc`, `realloc` | `kernel_lib/memory/malloc.c` | Extraction |
| `memcpy`, `memset`, `memmove`, `memcmp` | `kernel_lib/memory/string.c` | Extraction |
| `strlen`, `strcmp`, `strcpy`, `strncpy` | `kernel_lib/memory/string.c` | Extraction |

**Taux réutilisation**: 100% (juste déplacer dans nouveaux fichiers)

### CPU Features → kernel_lib/cpu/

| Fonctions Existantes | Destination | Notes |
|---------------------|-------------|-------|
| `rdtsc` (inline asm) | `kernel_lib/cpu/features.c` | Wrapper C |
| `cpuid` (inline asm) | `kernel_lib/cpu/features.c` | Wrapper C |
| IDT/PIC setup | `kernel_lib/cpu/interrupts.c` | Pour keyboard/timer |

**Taux réutilisation**: 90% (besoin wrappers simples)

### JIT Profiling → kernel_lib/jit/

| Fichier Actuel | Destination | Action | Taux Réutilisation |
|---------------|-------------|--------|-------------------|
| `kernel/adaptive_jit.{h,c}` | `kernel_lib/jit/profile.c` + `optimize.c` | Split en 2 fichiers | 95% |
| `kernel/function_profiler.{h,c}` | `kernel_lib/jit/profile.c` | Merge avec adaptive_jit | 90% |
| Profiling data structures | `kernel_lib/jit/profile.h` | Copie directe | 100% |

**Taux réutilisation global**: 95%

---

## 🔧 RÉUTILISABLE AVEC MODIFICATIONS MINEURES

### JIT Allocator (Phase 3)

| Fichier | Usage Future | Modifications |
|---------|-------------|---------------|
| `kernel/jit_allocator.{h,c}` | Phase 3 (Meta-circular JIT) | Garder pour plus tard |
| `kernel/jit_allocator_test.c` | Tests JIT allocator | Garder pour tests |

**Taux réutilisation**: 100% (pour Phase 3)

### LLVM Module Management (Phase 3)

| Fichier | Usage Future | Modifications |
|---------|-------------|---------------|
| `kernel/llvm_module_manager.{h,c}` | Phase 3 (charger .bc) | Garder archivé |
| `kernel/bitcode_module.{h,c}` | Phase 3 (parser bitcode) | Garder archivé |
| `kernel/micro_jit.{h,c}` | Phase 3 (mini JIT) | Garder archivé |

**Taux réutilisation**: 80-90% (besoin adaptations pour Phase 3)

---

## ❌ NE PAS RÉUTILISER (à archiver)

### Kernel Monolithique

| Fichier | Raison | Action |
|---------|--------|--------|
| `kernel/kernel.c` | Remplacé par `tinyllama/main.c` | Archiver (référence) |
| `kernel/embedded_modules.h` | Plus de modules embarqués | Supprimer |

### Module Loading (Phase 1 uniquement)

| Fichier | Raison | Action |
|---------|--------|--------|
| `kernel/module_loader.{h,c}` | Single binary (pas de modules) | Archiver pour Phase 3 |
| `kernel/disk_module_loader.{h,c}` | Pas besoin charger modules | Archiver pour Phase 3 |
| `kernel/elf_loader.{h,c}` | Pas besoin ELF loader | Archiver pour Phase 3 |
| `kernel/elf_test.{h,c}` | Tests ELF loader | Archiver |

### Tests & Demos (à conserver comme référence)

| Fichier | Usage | Action |
|---------|-------|--------|
| `kernel/llvm_test*.c` | Tests LLVM | Garder comme tests de validation |
| `kernel/jit_demo.{h,c}` | Demo JIT | Garder comme exemple |
| `kernel/fat16_test.{h,c}` | Tests FAT16 | Garder si on garde FAT16 |
| `kernel/function_profiler_test.c` | Tests profiler | Garder pour valider kernel_lib |

---

## 📦 MODULES SOURCES (Réutilisables comme Tests)

Les sources de modules dans `modules/` peuvent servir pour tester kernel_lib:

```bash
modules/compute.c       → Test compute-intensive
modules/fibonacci.c     → Test récursion
modules/primes.c        → Test boucles
modules/sum.c           → Test simple
modules/matrix_mul.c    → Test calcul matriciel
modules/sha256.c        → Test crypto
modules/fft_1d.c        → Test FFT
```

**Usage**: Compiler avec kernel_lib.a pour valider que tout fonctionne

---

## 🚀 Toolchain (100% Réutilisable)

### Build System

| Fichier | Destination | Modifications |
|---------|-------------|---------------|
| `Makefile` | `Makefile` | Adapter pour nouveau workflow |
| `linker.ld` | `linker.ld` | Adapter pour tinyllama_bare.elf |
| `boot/stage1.asm` | **Inchangé** | 100% réutilisable |
| `boot/stage2.asm` | **Inchangé** | 100% réutilisable |
| `boot/entry.asm` | Modifier jump | 95% réutilisable |

### LLVM Toolchain

```bash
# 100% réutilisable tel quel
clang-18
llvm-ar-18      # Pour créer kernel_lib.a
ld.lld-18
opt-18
llc-18
llvm-link-18    # Pour Phase 3 (link .bc files)
```

---

## 📊 Statistiques de Réutilisation

| Catégorie | Fichiers | Taux Réutilisation |
|-----------|----------|-------------------|
| **I/O Drivers** | 5 fichiers | 98% |
| **Memory Management** | stdlib.c | 100% (extraction) |
| **CPU Features** | inline asm | 90% |
| **JIT Profiling** | 4 fichiers | 95% |
| **Bootloader** | 3 fichiers asm | 100% |
| **Toolchain** | LLVM 18 | 100% |
| **Tests/Modules** | ~15 fichiers | 100% (référence) |

**TOTAL: ~80-85% du code est réutilisable!**

---

## 🎯 Plan d'Action (Session 18)

### Phase 1: Copie Directe (2h)
```bash
# I/O
cp kernel/vga.{h,c} kernel_lib/io/
cp kernel/keyboard.h kernel_lib/io/
cp kernel/fat16.{h,c} kernel_lib/io/  # optionnel

# CPU
cp kernel/pic.{h,c} kernel_lib/cpu/
cp kernel/idt.{h,c} kernel_lib/cpu/
```

### Phase 2: Extraction (2h)
```bash
# Memory (extraire de stdlib.c)
# → kernel_lib/memory/malloc.c
# → kernel_lib/memory/string.c

# Serial (extraire de kernel.c)
# → kernel_lib/io/serial.c
```

### Phase 3: Split & Merge (2h)
```bash
# JIT Profiling
# adaptive_jit.c + function_profiler.c
# → kernel_lib/jit/profile.c
# → kernel_lib/jit/optimize.c
```

### Phase 4: Build (1h)
```bash
# Créer Makefile.lib
# Compiler kernel_lib.a
# Valider taille ≤ 30KB
```

---

## 🔍 Fichiers à Analyser en Détail

Avant extraction, lire ces fichiers pour comprendre dépendances:

1. `kernel/stdlib.c` - Voir toutes les fonctions disponibles
2. `kernel/adaptive_jit.c` - Logique de profiling/optimization
3. `kernel/function_profiler.c` - Profiling per-function
4. `kernel/kernel.c` - Fonctions serial_* à extraire

---

**Auteur**: Session 17
**Status**: Document de référence pour Session 18
**Conclusion**: **80-85% du code est réutilisable**, principalement par extraction/copie
