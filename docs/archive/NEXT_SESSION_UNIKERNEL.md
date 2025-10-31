# Session 18 - Plan d'Action : Unikernel Refactor

**Date préparation** : 2025-10-25 (Session 17)
**Objectif** : Commencer l'extraction de kernel_lib.a (Semaine 1 du plan)

---

## 🎯 Objectif de la Session

**Créer kernel_lib.a** : Bibliothèque runtime minimale (~20-30KB) contenant :
- I/O : VGA, serial, keyboard
- Memory : malloc, string functions
- CPU : rdtsc, cpuid, features
- JIT : profiling, optimization

---

## 📋 Checklist des Tâches

### Phase 1 : Préparation (30 min)

- [ ] Créer répertoire `kernel_lib/` à la racine
- [ ] Créer sous-répertoires : `io/`, `memory/`, `cpu/`, `jit/`
- [ ] Lire fichiers existants à extraire :
  ```bash
  kernel/vga.{h,c}
  kernel/serial.c
  kernel/keyboard.h
  kernel/stdlib.{h,c}
  kernel/adaptive_jit.{h,c}
  ```

### Phase 2 : Extraction I/O (1h)

- [ ] **VGA** :
  - [ ] Copier `kernel/vga.h` → `kernel_lib/io/vga.h`
  - [ ] Copier `kernel/vga.c` → `kernel_lib/io/vga.c`
  - [ ] Retirer dépendances non-essentielles
  - [ ] Créer API publique minimale :
    ```c
    void vga_init(void);
    void vga_putchar(char c);
    void vga_puts(const char* str);
    void vga_clear(void);
    void vga_setcolor(uint8_t fg, uint8_t bg);
    ```

- [ ] **Serial** :
  - [ ] Créer `kernel_lib/io/serial.h`
  - [ ] Copier code serial depuis `kernel/serial.c` → `kernel_lib/io/serial.c`
  - [ ] API publique :
    ```c
    void serial_init(void);
    void serial_putchar(char c);
    void serial_puts(const char* str);
    int serial_getchar(void);  // Non-blocking
    ```

- [ ] **Keyboard** :
  - [ ] Copier `kernel/keyboard.h` → `kernel_lib/io/keyboard.h`
  - [ ] Simplifier si nécessaire
  - [ ] API publique :
    ```c
    void keyboard_init(void);
    int keyboard_getchar(void);  // Blocking
    int keyboard_available(void); // Check if key pressed
    ```

### Phase 3 : Extraction Memory (1h)

- [ ] **Malloc/Free** :
  - [ ] Extraire de `kernel/stdlib.c` → `kernel_lib/memory/malloc.c`
  - [ ] Créer `kernel_lib/memory/malloc.h`
  - [ ] Simplifier allocator (bump allocator suffit)
  - [ ] API publique :
    ```c
    void* malloc(size_t size);
    void free(void* ptr);
    void* calloc(size_t nmemb, size_t size);
    void* realloc(void* ptr, size_t size);
    ```

- [ ] **String Functions** :
  - [ ] Extraire de `kernel/stdlib.c` → `kernel_lib/memory/string.c`
  - [ ] Créer `kernel_lib/memory/string.h`
  - [ ] API publique :
    ```c
    void* memcpy(void* dst, const void* src, size_t n);
    void* memset(void* s, int c, size_t n);
    int memcmp(const void* s1, const void* s2, size_t n);
    size_t strlen(const char* s);
    int strcmp(const char* s1, const char* s2);
    char* strcpy(char* dst, const char* src);
    ```

### Phase 4 : Extraction CPU (45 min)

- [ ] **CPU Features** :
  - [ ] Créer `kernel_lib/cpu/features.h`
  - [ ] Créer `kernel_lib/cpu/features.c`
  - [ ] Implémenter rdtsc, cpuid wrappers
  - [ ] API publique :
    ```c
    uint64_t cpu_rdtsc(void);
    void cpu_cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx,
                   uint32_t* ecx, uint32_t* edx);
    int cpu_has_sse(void);
    int cpu_has_avx(void);
    ```

- [ ] **Interrupts** (optionnel) :
  - [ ] Créer `kernel_lib/cpu/interrupts.h`
  - [ ] Créer `kernel_lib/cpu/interrupts.c`
  - [ ] Setup IDT basique si nécessaire pour keyboard/timer

### Phase 5 : Extraction JIT Runtime (1h30)

- [ ] **Profiling** :
  - [ ] Créer `kernel_lib/jit/profile.h`
  - [ ] Créer `kernel_lib/jit/profile.c`
  - [ ] Extraire logic de profiling depuis `kernel/adaptive_jit.c`
  - [ ] API publique :
    ```c
    void jit_profile_begin(const char* func_name);
    void jit_profile_end(const char* func_name);
    uint64_t jit_get_call_count(const char* func_name);
    uint64_t jit_get_avg_cycles(const char* func_name);
    void jit_print_stats(const char* func_name);
    ```

- [ ] **Optimization** :
  - [ ] Créer `kernel_lib/jit/optimize.h`
  - [ ] Créer `kernel_lib/jit/optimize.c`
  - [ ] Extraire logic d'optimisation depuis `kernel/adaptive_jit.c`
  - [ ] API publique :
    ```c
    void jit_optimize_hot_functions(void);
    void jit_set_threshold(int warm, int hot, int very_hot);
    int jit_get_optimization_level(const char* func_name);
    void jit_reset_all(void);
    ```

- [ ] **Adaptive** :
  - [ ] Créer `kernel_lib/jit/adaptive.c`
  - [ ] Logique de détection des hot paths
  - [ ] Décisions de recompilation

### Phase 6 : API Publique Unifiée (30 min)

- [ ] **runtime.h** :
  - [ ] Créer `kernel_lib/runtime.h`
  - [ ] Include tous les headers I/O + Memory + CPU
  - [ ] Documentation des fonctions
  ```c
  #ifndef RUNTIME_H
  #define RUNTIME_H

  #include "io/vga.h"
  #include "io/serial.h"
  #include "io/keyboard.h"
  #include "memory/malloc.h"
  #include "memory/string.h"
  #include "cpu/features.h"

  #endif // RUNTIME_H
  ```

- [ ] **jit_runtime.h** :
  - [ ] Créer `kernel_lib/jit_runtime.h`
  - [ ] Include headers JIT
  - [ ] Documentation
  ```c
  #ifndef JIT_RUNTIME_H
  #define JIT_RUNTIME_H

  #include "jit/profile.h"
  #include "jit/optimize.h"

  #endif // JIT_RUNTIME_H
  ```

### Phase 7 : Build System (1h)

- [ ] **Makefile.lib** :
  - [ ] Créer `kernel_lib/Makefile.lib`
  - [ ] Compiler tous les .c en .o
  - [ ] Archiver en kernel_lib.a avec llvm-ar
  - [ ] Target: `make -f Makefile.lib`
  - [ ] Clean target

- [ ] **Validation** :
  - [ ] `make -f Makefile.lib` → success
  - [ ] `ls -lh kernel_lib.a` → size ≤ 30KB
  - [ ] `nm kernel_lib.a` → verify symbols exported

### Phase 8 : Tests Basiques (30 min)

- [ ] **Test Linking** :
  - [ ] Créer `test/test_runtime.c` :
    ```c
    #include "runtime.h"
    #include "jit_runtime.h"

    int main(void) {
        vga_init();
        vga_puts("Runtime library test\n");

        jit_profile_begin("test");
        // ... some work ...
        jit_profile_end("test");

        return 0;
    }
    ```
  - [ ] Compiler avec `-lkernel_lib`
  - [ ] Vérifier absence d'erreurs de linking

---

## 🔧 Commandes Clés

```bash
# Créer structure
mkdir -p kernel_lib/{io,memory,cpu,jit}

# Build library
cd kernel_lib
make -f Makefile.lib

# Vérifier taille
ls -lh kernel_lib.a

# Vérifier symboles
nm kernel_lib.a | grep " T "

# Test compilation
clang-18 -m32 -ffreestanding -nostdlib \
  -I../kernel_lib \
  test/test_runtime.c \
  -L../kernel_lib -lkernel_lib \
  -o test/test_runtime.elf
```

---

## ⚠️ Points d'Attention

1. **Dépendances circulaires** : S'assurer qu'aucune fonction de kernel_lib ne dépend de kernel.c
2. **Taille** : Viser ≤ 30KB pour kernel_lib.a
3. **API minimale** : Ne pas tout extraire, seulement l'essentiel
4. **Compatibilité** : Garder `-m32 -ffreestanding -nostdlib`
5. **Tests** : Valider chaque composant avant d'avancer

---

## 📊 Critères de Succès

À la fin de la session, nous devrions avoir :

- ✅ `kernel_lib/` directory avec structure complète
- ✅ `kernel_lib.a` compilé avec succès
- ✅ Taille ≤ 30KB
- ✅ API publique documentée (runtime.h, jit_runtime.h)
- ✅ Test de linking réussi
- ✅ Aucune dépendance vers kernel.c

---

## 🚀 Session Suivante (Session 19)

Si Session 18 réussit, Session 19 se concentrera sur :

1. Créer `tinyllama/` directory
2. Implémenter `main.c` basique
3. Linker avec kernel_lib.a
4. Générer tinyllama_bare.elf

---

**Préparé par** : Claude (Session 17)
**Temps estimé** : 6-8 heures de développement
**Difficulté** : Moyenne (extraction + refactoring)
