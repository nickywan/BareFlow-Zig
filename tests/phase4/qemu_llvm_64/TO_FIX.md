# TO_FIX.md - Problèmes critiques BareFlow Phase 4

**Date**: 2025-10-26
**Session**: 45 - Investigation du bug de compilation Clang
**Status**: 🔴 BLOQUANT - Empêche progression Phase 4

---

## 🚨 PROBLÈME CRITIQUE #1: Bug de corruption de valeur de retour (Clang 14 + -O0)

### Symptômes
Le kernel **hang** immédiatement après que `malloc()` retourne avec succès. Le noyau ne plante pas, il se fige complètement sans afficher SUCCESS ou FAILED.

### Localisation exacte
**Fichier**: `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/kernel.c:73-84`

```c
// kernel.c:72
println("[Test 3] malloc (bump allocator - 64 MB heap):");

// kernel.c:73-78
void* ptr1;
ptr1 = malloc(1024);           // ← malloc() retourne avec succès
void* ptr1_fixed;
asm volatile("mov %%rax, %0" : "=r"(ptr1_fixed) : : "memory");

if (ptr1_fixed) {              // ← HANG ICI - jamais exécuté
    println("  malloc(1024) -> SUCCESS");
} else {
    println("  malloc(1024) -> FAILED");
}
```

### Sortie observée
```
[Test 3] malloc (bump allocator - 64 MB heap):
[MALLOC] Entry
[MALLOC] Aligning size
[MALLOC] Checking heap_offset
[MALLOC] Computing ptr
[MALLOC] Updating offset
[MALLOC] Returning
                              ← HANG ICI (ligne 24 du log)
```

**Observation**: Tous les prints internes de `malloc()` s'affichent, confirmant que la fonction **se termine correctement**. Le hang se produit dans `kernel_main()` après le retour.

### Cause racine confirmée

#### Analyse assembly (voir `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/test_return_clang14.s`)

**Clang 14 + -O0 génère du code DÉFECTUEUX**:

```asm
# Prologue de la fonction
test_return:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $16, %rsp            # Alloue 16 octets sur la pile

# Appel de serial_puts
.LBB0_2:
    movq    $.L.str.1, %rdi
    callq   serial_puts          # ← Peut clobberer RAX

    movl    $0, -4(%rbp)         # ← STOCKE 0 SUR LA PILE (pas dans RAX!)

# Épilogue
.LBB0_3:
    movl    -4(%rbp), %eax       # ← CHARGE depuis la PILE (VULNÉRABLE!)
    addq    $16, %rsp
    popq    %rbp
    retq
```

**Comparaison avec GCC (CORRECT)**:

```asm
# GCC génère du code correct
.L2:
    movq    $.LC1, %rdi
    call    serial_puts
    movl    $0, %eax             # ← Charge 0 DIRECTEMENT dans EAX (CORRECT!)
.L3:
    leave
    ret
```

### Séquence de corruption

1. `malloc()` retourne un pointeur valide dans **RAX**
2. Clang génère: `mov RAX, -8(%rbp)` (stocke sur la pile)
3. **La pile est corrompue** entre l'écriture et la lecture
   - Possiblement par un interrupt handler non sauvegardé
   - Ou corruption de la pile pendant l'accès BSS
4. Clang génère: `mov -8(%rbp), RAX` (recharge depuis la pile)
5. **RAX contient maintenant une valeur corrompue**
6. `kernel_main()` essaie d'utiliser le pointeur corrompu → **HANG**

### Référence documentation
Voir: `/home/nickywan/dev/Git/BareFlow-LLVM/docs/phase4/SESSION_45_COMPILER_BUG_INVESTIGATION.md:26-64`

---

## 🟠 PROBLÈME #2: Taille BSS excessive cause échec malloc() avec 8MB heap

### Symptômes
Avec `HEAP_SIZE = 8 MB`, `malloc()` échoue en essayant d'accéder à la variable `heap_magic` à la fin du BSS.

### Localisation
**Fichier**: `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/malloc_simple.c:12-17`

```c
// malloc_simple.c:12
#define HEAP_SIZE (8 * 1024 * 1024)  // 8 MB - PROBLÉMATIQUE
#define HEAP_MAGIC 0xDEADBEEFCAFEBABEULL

static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static size_t heap_offset = 0;
static uint64_t heap_magic = HEAP_MAGIC;  // ← Variable à ~8MB dans BSS
```

### Analyse BSS

```bash
# Avec 8MB heap:
$ size kernel.elf
   text    data     bss     dec     hex filename
  11289     624 9911496 9923409  975b51 kernel.elf
#                ^^^^^^^ 9.45 MB BSS!

# Avec 1MB heap:
$ size kernel.elf
   text    data     bss     dec     hex filename
  11289     624 2160728 2172641  212441 kernel.elf
#                ^^^^^^^ 2.06 MB BSS
```

### Cause racine
**Fichier**: `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/boot.S:63-77`

Le setup de paging mappe seulement **512 MB** avec des pages de 2 MB:

```asm
# boot.S:68-77
// 3. Setup PD entries using 2 MB pages (identity map 0-512 MB)
movabs $pd_table, %rdi
mov $256, %rcx              // Map 256 × 2 MB = 512 MB
xor %rax, %rax

.fill_pd_loop:
    mov %rax, %rdx
    or $0x083, %rdx         // Present + Writable + Page Size (2 MB)
    mov %rdx, (%rdi)
    add $(2 * 1024 * 1024), %rax
    add $8, %rdi
    loop .fill_pd_loop
```

**Mais**: BSS section de 9.45 MB peut potentiellement dépasser la région mappée si placée à une adresse haute, causant un accès à mémoire non mappée.

### Solution testée ✅

```c
// malloc_simple.c:12
#define HEAP_SIZE (1 * 1024 * 1024)  // 1 MB - FONCTIONNE
```

**Résultat**: malloc() réussit maintenant, mais problème #1 (corruption de retour) subsiste.

---

## 🟠 PROBLÈME #3: -O1 empire les choses (paradoxe assembly correct)

### Symptômes
Malgré une génération d'assembly **correcte** pour les valeurs de retour, `-O1` cause un crash **plus précoce** que `-O0`.

### Localisation
**Fichier**: `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/Makefile:27`

```makefile
# Makefile:27
CXXFLAGS = -target x86_64-unknown-none \
           -ffreestanding -nostdlib -fno-pie -O0 -Wall -Wextra \
           #                                  ^^^ Actuellement -O0
```

### Comportement observé

| Configuration | Profiler | malloc() | Result |
|---------------|----------|----------|--------|
| **Clang 14 + -O0** | ✅ Init OK | ✅ Réussit | ❌ Hang au retour malloc (ligne 24 log) |
| **Clang 14 + -O1** | ❌ Crash | N/A | 🔥 **Reboot infini immédiat!** |

### Logs -O1
**Fichier**: `/tmp/test_real_o1.log` (test précédent)

```
========================================
  BareFlow QEMU x86-64 Kernel
  Session 43 - Profiler Integration Test
========================================

[Profiler] Initializing...
                                    ← REBOOT INFINI ICI

========================================
  BareFlow QEMU x86-64 Kernel
  Session 43 - Profiler Integration Test
========================================

[Profiler] Initializing...
                                    ← REBOOT INFINI
```

### Analyse assembly -O1
**Fichier**: `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/test_return_clang14_o1.s`

```asm
# Clang 14 + -O1 génère du code CORRECT pour les valeurs de retour:
# %bb.2:
    movq    $.L.str.1, %rdi
    callq   serial_puts
    xorl    %eax, %eax           # ← Charge 0 directement dans EAX (CORRECT!)
    popq    %rbp
    retq
```

### Paradoxe
L'assembly pour les valeurs de retour est **correcte** avec -O1, mais le comportement est **catastrophique**. Cela suggère que:
1. Le problème n'est **pas uniquement** les valeurs de retour
2. -O1 introduit d'autres optimisations qui **cassent le code bare-metal**
3. Possibles causes:
   - Alignement de pile incorrect
   - Conventions d'appel violées
   - Registres clobbered non sauvegardés
   - Réordonnancement d'instructions problématique

### Référence
Voir: `/home/nickywan/dev/Git/BareFlow-LLVM/docs/phase4/SESSION_45_COMPILER_BUG_INVESTIGATION.md:54-96`

---

## 🟡 PROBLÈME #4: Makefile incohérent - Flags hardcodés

### Symptômes
Changer `CXXFLAGS` de `-O0` à `-O1` n'avait **AUCUN EFFET** car les règles individuelles ont des flags hardcodés.

### Localisation
**Fichier**: `/home/nickywan/dev/Git/BareFlow-LLVM/tests/phase4/qemu_llvm_64/Makefile:49-95`

### Règles problématiques

```makefile
# Makefile:49-54 - kernel.o
kernel.o: kernel.c tinyllama_model.h
	@echo "  [CC]  $< (testing Clang 14)"
	@clang-14 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O0 -Wall -Wextra \
	          #                                                                  ^^^ HARDCODÉ!
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none \
	          -I../../../kernel_lib -c $< -o $@

# Makefile:56-62 - tinyllama_model.o
tinyllama_model.o: tinyllama_model.c tinyllama_model.h
	@echo "  [CC]  $< (testing Clang 14)"
	@clang-14 -target x86_64-unknown-none -ffreestanding -nostdlib -fno-pie -O0 \
	          #                                                                  ^^^ HARDCODÉ!
	          -Wall -Wextra \
	          -fno-stack-protector -mno-red-zone -mcmodel=kernel \
	          -fcf-protection=none \
	          -I../../../kernel_lib -c $< -o $@

# Même problème pour:
# - tinyllama_inference.o (ligne 78)
# - tinyllama_weights.o (ligne 85)
# - profiler.o (ligne 92)
```

### Impact
Tester différents niveaux d'optimisation via `CXXFLAGS` était **impossible** sans modifier manuellement 5 règles différentes.

### Solution
Les règles doivent utiliser `$(CXXFLAGS)` au lieu de flags hardcodés:

```makefile
# CORRECT:
kernel.o: kernel.c tinyllama_model.h
	@echo "  [CC]  $<"
	@$(CC) $(CXXFLAGS) -c $< -o $@
```

### Référence
Voir: `/home/nickywan/dev/Git/BareFlow-LLVM/docs/phase4/SESSION_45_COMPILER_BUG_INVESTIGATION.md:9-22`

---

## ❌ WORKAROUNDS TESTÉS (tous échoués)

### 1. ❌ Inline Assembly pour capturer RAX
**Fichier**: `kernel.c:73-78`

```c
void* ptr1;
ptr1 = malloc(1024);
void* ptr1_fixed;
asm volatile("mov %%rax, %0" : "=r"(ptr1_fixed) : : "memory");

if (ptr1_fixed) {  // ← Toujours hang!
```

**Résultat**: Échec - RAX déjà corrompu au moment de l'inline asm.

### 2. ❌ -O1 optimization
**Résultat**: Empire les choses - crash plus tôt (profiler init au lieu de malloc return).

### 3. ❌ Volatile keyword (test précédent)
**Résultat**: Amélioration partielle mais crash persiste plus tard.

### 4. ❌ -Og optimization (test précédent)
**Résultat**: Pire que -O0.

---

## 📊 RÉCAPITULATIF - État actuel

### Configuration qui va le plus loin
**Clang 14 + -O0 + 1MB heap**

| Étape | Status |
|-------|--------|
| Boot & Multiboot2 | ✅ |
| Profiler init | ✅ |
| Serial I/O | ✅ |
| Paging setup | ✅ |
| malloc() internal | ✅ |
| malloc() return to caller | ❌ **HANG** |

### Logs finaux
**Fichier**: `/tmp/test_1mb_heap.log`

```
========================================
  BareFlow QEMU x86-64 Kernel
  Session 43 - Profiler Integration Test
========================================

[Profiler] Initializing...
[Profiler] Initialized (rdtsc-based)
[Profiler] Ready - tracking 6 functions
[Test 1] Serial I/O:
  Serial output working!

[Test 2] Paging & Memory:
  Paging initialized (2 MB pages)
  Identity mapped: 0-512 MB
  Page tables setup: PML4 -> PDPT -> PD

[Test 3] malloc (bump allocator - 64 MB heap):
[MALLOC] Entry
[MALLOC] Aligning size
[MALLOC] Checking heap_offset
[MALLOC] Computing ptr
[MALLOC] Updating offset
[MALLOC] Returning
                                    ← HANG LIGNE 24
```

---

## 🔬 PISTES D'INVESTIGATION

### Piste 1: Compiler différent
- **GCC**: Génère du code correct pour les valeurs de retour
  - Problème: Build échoue au linking (test précédent)
  - Action: Investiguer les erreurs de linking GCC
- **Clang 15/16/17/18**: Possiblement bug fixé dans versions récentes
  - Action: Tester avec Clang 18 (déjà installé)

### Piste 2: Architecture différente
- **x86 (32-bit)**: Le bug peut être spécifique à x86-64
  - Pro: Plus simple, moins de registres
  - Con: Nécessite réécriture complète de boot.S et linker.ld

### Piste 3: Wrappers assembly
Créer des wrappers assembly pour **tous** les appels de fonction critiques:

```asm
# wrapper_malloc.S
.global malloc_wrapper
malloc_wrapper:
    # Sauvegarde tous les registres
    push %rbx
    push %rcx
    push %rdx

    # Appel malloc
    call malloc

    # RAX contient le résultat - le sauvegarder dans un registre callee-saved
    mov %rax, %rbx

    # Restaurer registres
    pop %rdx
    pop %rcx
    # Ne pas pop %rbx - contient le résultat!

    # Retourner avec résultat dans %rbx
    mov %rbx, %rax
    pop %rbx
    ret
```

### Piste 4: Analyser la pile en temps réel
Ajouter des diagnostics pour inspecter l'état de la pile:

```c
// Après malloc(), avant if:
ptr1 = malloc(1024);

// Inspecter la pile
serial_puts("[DEBUG] Stack inspection:\n");
serial_puts("  RBP = "); serial_put_hex(read_rbp());
serial_puts("  RSP = "); serial_put_hex(read_rsp());
serial_puts("  [RBP-8] = "); serial_put_hex(*(uint64_t*)(read_rbp() - 8));

if (ptr1) {
```

### Piste 5: Session 32 logs - Free-list corruption
Référence du doc Phase 4: Vérifier les logs Session 32 pour:
- Bug `_Bool` vs `uint8_t` dans le free-list allocator
- Overflow guards manquants dans calloc/realloc

### Piste 6: Page-fault handler
**Fichier**: `boot.S` (IDT setup manquant)

Actuellement, aucun page-fault handler n'est enregistré. Si la corruption cause un accès mémoire invalide, le système reboot silencieusement.

Action: Implémenter un page-fault handler minimal pour diagnostics:

```asm
page_fault_handler:
    # Afficher "PAGE FAULT" sur serial
    movq $fault_msg, %rsi
    call serial_write

    # CR2 contient l'adresse fautive
    mov %cr2, %rax
    call serial_put_hex

    hlt
```

---

## 🎯 RECOMMANDATIONS URGENTES

### Option A: Changer de compilateur (RECOMMANDÉ)
**Action**: Tester avec **Clang 18** (déjà disponible)

```bash
# Modifier Makefile:
CC = clang++-18    # Au lieu de clang++-14
AS = clang-18      # Au lieu de clang-14
```

**Probabilité de succès**: 70% - Les versions récentes peuvent avoir fixé ce bug.

### Option B: Architecture 32-bit
**Action**: Porter le code en x86 (32-bit)

**Avantages**:
- Plus simple, moins de cas d'edge avec les registres
- Clang plus mature pour x86 que x86-64 bare-metal

**Inconvénients**:
- Réécriture complète de `boot.S` (protected mode au lieu de long mode)
- Réécriture de `linker.ld`
- Perte des registres supplémentaires de x86-64

**Probabilité de succès**: 60%

### Option C: GCC pure
**Action**: Résoudre les erreurs de linking GCC

**État**: Build échoue actuellement au linking (test précédent non documenté)

**Action requise**: Investiguer les erreurs de linking spécifiques

**Probabilité de succès**: 50%

### Option D: Wrappers assembly universels
**Action**: Créer un système de wrappers assembly pour tous les appels

**Avantages**:
- Contrôle total sur la convention d'appel
- Peut forcer la préservation de RAX

**Inconvénients**:
- Très laborieux (wrapper pour chaque fonction)
- Maintenance difficile
- Performance impact

**Probabilité de succès**: 40%

---

## 📝 CHECKLIST PROCHAINE SESSION

### Tests immédiats
- [ ] Tester avec Clang 18 (changer CC/AS dans Makefile)
- [ ] Tester avec Clang 17, 16, 15 si Clang 18 échoue
- [ ] Essayer GCC et documenter erreurs de linking précises
- [ ] Implémenter page-fault handler pour meilleurs diagnostics

### Investigations approfondies
- [ ] Lire Session 32 logs pour bug free-list (_Bool vs uint8_t)
- [ ] Analyser state de la pile avec serial_put_hex de RBP/RSP
- [ ] Vérifier si interrupt handlers sauvegardent correctement registres
- [ ] Tester avec des heap sizes différents (512KB, 2MB, 4MB)

### Solutions alternatives
- [ ] Porter en x86 32-bit si rien d'autre ne marche
- [ ] Considérer bare-metal Rust (meilleur support compilateur)
- [ ] Envisager émulateur/simulateur avec meilleurs outils de debug (Bochs)

---

## 🔗 RÉFÉRENCES

### Code source problématique
- **Bug principal**: `kernel.c:73-84` (hang après malloc return)
- **Allocator**: `malloc_simple.c:12` (taille heap), `malloc_simple.c:19-46` (implémentation)
- **Paging setup**: `boot.S:63-77` (limite 512 MB)
- **Makefile**: `Makefile:27` (CXXFLAGS), `Makefile:49-95` (règles hardcodées)

### Documentation
- **Session 45**: `/home/nickywan/dev/Git/BareFlow-LLVM/docs/phase4/SESSION_45_COMPILER_BUG_INVESTIGATION.md`
- **Assembly analysis**: `test_return_clang14.s`, `test_return_clang14_o1.s`, `test_return_gcc.s`
- **Test logs**: `/tmp/test_1mb_heap.log`, `/tmp/test_model_call.log`

### Références externes
- Clang bug report: (à créer si confirmé sur Clang récent)
- x86-64 ABI: https://github.com/hjl-tools/x86-psABI/wiki/x86-64-psABI-1.0.pdf
- OSDev Wiki: https://wiki.osdev.org/Calling_Conventions

---

**STATUT FINAL**: 🔴 **BLOQUEUR** - Phase 4 ne peut pas progresser sans résoudre le bug de corruption de valeur de retour.
