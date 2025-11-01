# Issues Resolved - Index

**Objectif**: Base de données des problèmes rencontrés et leurs solutions.

**Organisation**: Un fichier par problème pour faciliter la consultation ciblée.

**Utilisation**: Consulter l'issue spécifique AVANT d'implémenter de nouvelles fonctionnalités ou lors du debugging.

---

## Issues Critiques (🔴)

### Boot Issues
- **[Issue #1](issue-01-boot-failures.md)**: Kernel ne boot pas - CPU reset immédiat
  - 1.1: Multiboot2 Header Invalide
  - 1.2: BSS Non-Zeroed (Pages Tables Corrompues)
  - 1.3: Mismatch 32-bit Boot / 64-bit Kernel
  - 1.4: Stack Overflow / Red Zone Corruption
  - **Sessions**: 46, 47

### Compilation Issues
- **[Issue #2](issue-02-32bit-addressing.md)**: Zig génère des adresses 32-bit au lieu de 64-bit
  - Symptôme: Infinite 'E' characters, garbage data
  - Solution: `-mcmodel=kernel` flag
  - **Session**: 47 Part 2

### Runtime Issues
- **[Issue #14](issue-14-hex-printing-corruption.md)**: Hex Printing Corruption - Conditional Toujours True
  - Symptôme: Hex digits A-F affichent '<', '>', '?'
  - Cause: Zig ReleaseFast optimizer casse les conditionnels
  - Solution: Lookup table (const array)
  - **Session**: 47 (Continuation)
  - ⚠️ **IMPORTANT**: Ce bug cachait aussi le problème #15

- **[Issue #15](issue-15-32mb-heap-triple-fault.md)**: 32MB Heap Triple Fault
  - Symptôme: Kernel reboot 3× avec 32MB heap
  - Solution: Résolu par le fix de l'issue #14 (lookup table)
  - **Session**: 47 (Continuation)
  - ⚠️ **IMPORTANT**: Lié à l'issue #14

---

## Issues Moyennes (🟡)

### Compilation
- **Issue #3**: Property 'code_model' n'existe pas dans Zig 0.13
- **Issue #4**: Cannot use addCSourceFile for .zig files

### Runtime
- **Issue #5**: Serial I/O - Infinite 'E' characters (voir Issue #2)
- **Issue #6**: Serial Polling Too Strict
- **Issue #7**: VGA Buffer Addressing

### Linker
- **Issue #8**: Section Alignment Syntax Error
- **Issue #9**: BSS Boundaries Not Defined
- **Issue #10**: PIE (Position Independent Executable) Enabled

### QEMU/Testing
- **Issue #11**: QEMU ne démarre pas avec -kernel
- **Issue #12**: Serial Output Non Visible
- **Issue #13**: ISO Checksum Different

---

## Quick Reference - Par Symptôme

### "Kernel ne boot pas"
→ [Issue #1](issue-01-boot-failures.md)

### "Infinite characters / Garbage output"
→ [Issue #2](issue-02-32bit-addressing.md) ou [Issue #14](issue-14-hex-printing-corruption.md)

### "Hex digits incorrects"
→ [Issue #14](issue-14-hex-printing-corruption.md)

### "Triple fault / Reboot avec large heap"
→ [Issue #15](issue-15-32mb-heap-triple-fault.md)

---

## Workflow de Consultation

### Avant d'implémenter une nouvelle fonctionnalité
1. Identifier la catégorie (Boot, Compilation, Runtime, Linker, QEMU)
2. Lire les issues critiques (🔴) de cette catégorie
3. Appliquer les solutions proactivement

### Lors du debugging
1. Identifier le symptôme dans "Quick Reference"
2. Lire l'issue correspondante
3. Appliquer la validation décrite
4. Si nouveau problème, créer un nouveau fichier dans ce dossier

---

## Contribuer

### Créer une nouvelle issue

1. **Créer le fichier**: `issue-XX-short-title.md`

2. **Format standard**:
```markdown
# Issue #XX: Titre Court du Problème

**Catégorie**: Boot/Compilation/Runtime/Linker/QEMU
**Sévérité**: 🔴 CRITIQUE / 🟡 MOYEN / 🟢 FAIBLE
**Session**: XX

---

## Symptômes
...

## Cause Racine
...

## Solution
...

## Validation
...

## Fichiers Concernés
...

## Référence
...
```

3. **Ajouter à ce README**: Lien dans la section appropriée

4. **Mettre à jour CLAUDE.md**: Si pattern récurrent

---

## Niveaux de Sévérité

- 🔴 **CRITIQUE**: Bloque le boot ou cause data loss
- 🟡 **MOYEN**: Impacte fonctionnalité mais workaround existe
- 🟢 **FAIBLE**: Inconvénient mineur

---

## Historique des Sessions

| Session | Issues Résolues | Documentation |
|---------|----------------|---------------|
| 46 | #1 (Boot failures multiples) | `SESSION_46_FINAL.md` |
| 47-1 | #1.3 (32-bit/64-bit mismatch) | `SESSION_47_BREAKTHROUGH.md` |
| 47-2 | #2 (String pointer bug) | `SESSION_47_STRING_POINTER_FIX.md` |
| 47-3 | #14, #15 (Hex printing + 32MB heap) | `SESSION_47_ALLOCATOR_FIXES.md` |

---

**Dernière Mise à Jour**: 2025-11-01 (Session 47)
**Maintenu par**: Claude Code + @nickywan
