# Architecture 64-bit - BareFlow-Zig

**Question**: Est-on en 32 puis en 64 bit ou on est en full 64bits?

**Réponse**: ✅ **FULL 64-bit depuis le début** - Aucune transition 32→64!

---

## État Actuel (Session 47+)

### Boot Sequence

```
┌─────────────┐
│ BIOS/UEFI   │ (16-bit real mode)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ GRUB        │ Charge kernel + Multiboot2
│ Multiboot2  │ Active 64-bit long mode
└──────┬──────┘ Configure pagination 4-level
       │
       │ ✅ GRUB nous met en 64-bit!
       │
       ▼
┌─────────────┐
│ boot64.S    │ .code64 ← DÉJÀ en 64-bit!
│ _start      │ - Setup stack (RSP, pas ESP)
│             │ - Zero BSS (MOVABS, pas MOV)
│             │ - Setup page tables (RDI, RAX)
│             │ - Call kernel_main
└──────┬──────┘
       │
       │ ✅ Call 64-bit
       │
       ▼
┌─────────────┐
│ kernel_main │ Zig 64-bit natif
│ (Zig)       │ - 64-bit pointers (u64)
│             │ - 64-bit registers (RDI, RSI, RDX)
│             │ - Code model: kernel
└─────────────┘
```

### Registres Utilisés

**Partout en 64-bit**:
- **Stack**: RSP (64-bit), pas ESP (32-bit)
- **Pointeurs**: RDI, RSI, RDX (64-bit), pas EDI, ESI, EDX (32-bit)
- **Retour fonction**: RAX (64-bit), pas EAX (32-bit)
- **Base pointer**: RBP (64-bit), pas EBP (32-bit)

### Preuve: boot64.S Ligne 38

```assembly
.section .text
.code64              # ← DIRECTIVE ASSEMBLEUR: Code 64-bit!
.global _start
.type _start, @function
_start:
    mov $stack_top, %rsp    # ← RSP (64-bit), pas ESP
    movabs $pml4, %rax      # ← MOVABS (64-bit), pas MOV
    call kernel_main        # ← Call avec convention 64-bit
```

---

## Importance pour la Suite

### ✅ Avantages 64-bit Natif

1. **Pas de transition 32→64 à gérer**
   - Pas de GDT à configurer manuellement
   - Pas de mode protégé 32-bit
   - Pas de passage en long mode
   - GRUB fait tout le travail!

2. **Adressage mémoire complet**
   - Adresses 64-bit: `0x0000000000000000` à `0xFFFFFFFFFFFFFFFF`
   - Pas de limitation 4GB (32-bit)
   - Kernel peut être en higher-half: `0xFFFFFFFF80000000+`

3. **Registres supplémentaires**
   - x86-64 a **16 registres** (vs 8 en 32-bit)
   - R8, R9, R10, R11, R12, R13, R14, R15
   - Meilleure performance pour JIT/LLVM

4. **ABI System V AMD64**
   - Convention d'appel standardisée
   - Arguments: RDI, RSI, RDX, RCX, R8, R9
   - Retour: RAX, RDX (pour 128-bit)

### 🔴 Contraintes 64-bit à Respecter

1. **Code Model OBLIGATOIRE**
   ```bash
   # ❌ CASSÉ - Génère EDI (32-bit)
   zig build-obj src/main.zig

   # ✅ CORRECT - Génère RDI (64-bit)
   zig build-obj -mcmodel=kernel src/main.zig
   ```

2. **Red Zone à Désactiver**
   ```bash
   # Kernel 64-bit - TOUJOURS désactiver red zone
   -mno-red-zone
   ```

   **Pourquoi**: Red zone (128 bytes sous RSP) peut être corrompu par interrupts

3. **PIE à Désactiver**
   ```bash
   # Kernel ne doit PAS être relocatable
   -fno-pie
   kernel.pie = false
   ```

4. **MOVABS pour Adresses Absolues**
   ```assembly
   # ✅ CORRECT - 64-bit address load
   movabs $0x101000, %rdi

   # ❌ ÉVITER - Truncate à 32-bit
   mov $0x101000, %edi     # EDI = 32-bit!
   ```

---

## Ancien Système (boot.S) - OBSOLÈTE

**⚠️ Ce fichier N'EST PLUS UTILISÉ depuis Session 47**

### Ancienne Séquence (CASSÉE)

```
GRUB (32-bit) → boot.S (.code32) → Transition → Long Mode → kernel (64-bit)
                       ↓
                  ❌ INCOMPATIBLE!
                  boot.S charge kernel 64-bit
                  mais reste en 32-bit
```

### Pourquoi c'était cassé

1. **boot.S était en 32-bit** (`.code32`)
2. **Kernel Zig était en 64-bit**
3. **Mismatch**: 32-bit bootloader → 64-bit kernel
4. **Résultat**: Triple fault, reset, ou hang

### Fix (Session 47 Breakthrough)

**Remplacement**: boot.S → boot64.S
- Suppression de toute la transition 32→64
- GRUB Multiboot2 nous met directement en 64-bit
- boot64.S commence directement en `.code64`

**Commit**: f34f03d - "BREAKTHROUGH: 32-bit/64-bit boot mismatch resolved"

---

## Validation 64-bit

### 1. Vérifier Disassembly

```bash
objdump -d iso/boot/kernel | grep -A10 kernel_main
```

**Ce qu'on doit voir**:
```assembly
# ✅ CORRECT - Registres 64-bit
mov %rdi, %rax     # RDI (64-bit)
mov %rsi, %rcx     # RSI (64-bit)
movabs $0x..., %rdi # MOVABS (64-bit literal)

# ❌ CASSÉ - Registres 32-bit
mov %edi, %eax     # EDI (32-bit) - MAUVAIS!
mov %esi, %ecx     # ESI (32-bit) - MAUVAIS!
```

### 2. Vérifier Code Zig

```zig
// ✅ Types 64-bit natifs
const ptr: *u8 = @ptrFromInt(0x101000);  // 64-bit pointer
const addr: usize = 0x101000;            // usize = u64 sur x86-64

// Inline assembly - Utiliser registres 64-bit
asm volatile ("mov %[val], %rdi"  // RDI (64-bit)
    : [val] "r" (value)
);
```

### 3. Vérifier Compilation Flags

```bash
# build.zig ou commandes manuelles
-target x86_64-freestanding    # ✅ x86_64 (64-bit)
-mcmodel=kernel                # ✅ 64-bit addressing
-mno-red-zone                  # ✅ Kernel 64-bit
```

---

## Implications pour la Suite

### Phase Actuelle: Kernel Simple

**Status**: ✅ Fonctionne en 64-bit natif
- Serial I/O: 64-bit pointers
- VGA buffer: 64-bit addressing
- Stack: 64KB avec RSP
- Pagination: 4-level (PML4 → PDPT → PD → PT)

### Phase 5: Allocateur Mémoire (32MB Heap)

**Ce qui change**:
```zig
// malloc_llvm.c - Déjà en 64-bit!
static uint8_t heap[32 * 1024 * 1024];  // 32MB statique

// Pointeurs 64-bit
void* malloc(size_t size) {
    // size est size_t = u64 sur x86-64
    // Retour: void* = 64-bit pointer
}
```

**Pas de changement architecture nécessaire!**

### Phase 6: LLVM JIT Integration

**Bénéfices 64-bit**:
- LLVM natif 64-bit (libLLVM-18.so)
- JIT génère code x86-64 natif
- 16 registres disponibles (vs 8 en 32-bit)
- Meilleure performance vectorielle (AVX2, AVX-512)

**Compilation LLVM**:
```cpp
// LLVM Triple
llvm::Triple triple("x86_64-unknown-none-elf");  // 64-bit
// Code model
llvm::CodeModel::Kernel  // Higher-half kernel
```

### Phase 7: TinyLlama (~60MB Model)

**Adressage mémoire**:
- Model: 60MB → Adresse 64-bit requise
- Weights: Pointeurs 64-bit vers tenseurs
- Pas de limitation 4GB!

```zig
// Exemple: Load model weights
const weights: [*]f32 = @ptrFromInt(0x10000000);  // 64-bit pointer
const weights_size: usize = 60 * 1024 * 1024;     // u64 size
```

---

## Architecture Decision Record (ADR)

### Décision: Native 64-bit (Session 47)

**Contexte**:
- boot.S (32-bit) incompatible avec kernel Zig (64-bit)
- Triple faults et resets inexpliqués

**Options Considérées**:
1. ❌ Réécrire kernel en 32-bit → Perte des bénéfices 64-bit
2. ❌ Fixer transition 32→64 dans boot.S → Complexe, error-prone
3. ✅ **Utiliser GRUB Multiboot2 64-bit direct** → Simple, natif

**Décision**: Option 3 - Native 64-bit avec boot64.S

**Conséquences**:
- ✅ Plus simple (pas de transition)
- ✅ GRUB gère pagination et long mode
- ✅ Meilleure performance
- ✅ Compatible LLVM natif
- ⚠️ Requiert `-mcmodel=kernel` partout
- ⚠️ Requiert `-mno-red-zone` partout

---

## Checklist: Nouveau Code 64-bit

Avant d'ajouter du nouveau code, vérifier:

- [ ] **Compilation**: Flags 64-bit corrects
  - [ ] `-target x86_64-freestanding`
  - [ ] `-mcmodel=kernel`
  - [ ] `-mno-red-zone`
  - [ ] `-fno-pie`

- [ ] **Assembly Inline**: Registres 64-bit
  - [ ] Utiliser RDI, RSI, RDX (pas EDI, ESI, EDX)
  - [ ] Utiliser RSP (pas ESP)
  - [ ] Utiliser MOVABS pour adresses absolues

- [ ] **Pointeurs**: Types 64-bit
  - [ ] `usize` = 64-bit sur x86-64
  - [ ] `*T` = 64-bit pointer
  - [ ] Éviter casts vers u32

- [ ] **Testing**: Vérifier disassembly
  - [ ] `objdump -d` → Voir RDI/RSI/RDX
  - [ ] Pas de EDI/ESI/EDX dans kernel code
  - [ ] MOVABS pour adresses > 32-bit

---

## Résumé Exécutif

**Question**: 32-bit puis 64-bit, ou full 64-bit?

**Réponse**: ✅ **FULL 64-bit depuis GRUB!**

```
GRUB → boot64.S (.code64) → kernel_main (Zig 64-bit)
       └─ Native x86-64, aucune transition
```

**Impact pour la suite**:
- ✅ Continuer en 64-bit natif
- ✅ Toujours compiler avec `-mcmodel=kernel`
- ✅ Architecture stable jusqu'à TinyLlama
- ✅ Performance optimale (16 registres, AVX2)

**Pas de changement architecture prévu!**

---

**Dernière Mise à Jour**: 2025-11-01 (Session 47)
**Maintenu par**: Claude Code + @nickywan
**Version**: 1.0
