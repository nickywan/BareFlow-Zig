# Revue: ZIG_MIGRATION_STRATEGY vs État Actuel du Projet

Date: 2025-10-31
Projet: BareFlow-Zig (unikernel JIT x86-64)

**Verdict**: Accepté avec ajustements (fortement recommandé) — la direction est bonne et alignée avec les problèmes rencontrés, mais plusieurs points du plan Zig doivent être adaptés au contexte bare‑metal actuel et à l’architecture du dépôt.

---

**Contexte Court**
- Code actuel majoritairement en C/C++ bare‑metal, aucun fichier Zig présent.
- 64‑bit: validé et pratiqué sous `tests/phase4/qemu_llvm_64` (GRUB/Multiboot2, QEMU x86_64). `kernel_lib/Makefile.llvm` est 64‑bit; en revanche `tinyllama/Makefile` reste 32‑bit (i386).
- Problèmes mémoire en C documentés (malloc/free‑list) ⟶ cause principale de l’effort de migration.
- Document audité: `ZIG_MIGRATION_STRATEGY.md` (31 Oct 2025, Phase 46+).

---

**Points Forts Du Plan Zig**
- Cible les vrais pain points (sécurité mémoire, BSS/heap, erreurs explicites).
- Phasage progressif et compatible QEMU (boot minimal + drivers + modèle TinyLlama + JIT).
- FFI C↔Zig simple et robuste pour cohabiter avec `kernel_lib` durant la transition.
- Alignement avec la vision « Grow to Shrink » (instrumentation comptime, réécriture ciblée, réduction de code).

---

**Écarts/Bloquants À Corriger**
1) Lien LLVM en environnement freestanding
- Le plan propose `kernel.linkSystemLibrary("LLVM-18")`. En bare‑metal il n’y a pas de « system library ». Il faut des archives statiques LLVM cross‑compilées pour `.freestanding` x86‑64, et un memory manager adapté (ORC/JIT alloue/exécute mémoire).
- Reco: d’abord valider JIT hors noyau Zig (scénarios userspace/tests existants) ou via une fine surcouche C déjà éprouvée, puis exposer une API minimale à Zig. Reporter l’intégration ORC complète après paging + permissions mémoire (RX/RW) en place.

2) Usage de la std Zig en freestanding
- Les extraits utilisent `std.heap.page_allocator` et des helpers de fichiers (`loadFile`) qui ne sont pas disponibles sans OS. En bare‑metal, partir sur `FixedBufferAllocator` (ou un allocateur custom) et bannir toute I/O fichier tant que le stockage n’est pas implémenté.
- Reco: définir un tampon statique aligné pour le heap et encapsuler un allocateur Zig au‑dessus (pas de page allocator implicite).

3) Entrée/Boot Multiboot2 en Zig
- L’exemple de header Multiboot2 en Zig est simplifié. En pratique il faut: section, alignement, checksum exact, puis un stub ASM pour stack/GDT et le saut vers `kernel_main`. Le dépôt possède déjà des stubs GRUB fonctionnels (`tests/phase4/qemu_llvm_64/boot.S`).
- Reco: réutiliser temporairement l’ASM existant et linker l’obj Zig (ou générer l’obj Zig et laisser GRUB/boot.S init). La migration du header 100% Zig peut venir plus tard.

4) Mismatch d’architecture dans le dépôt
- `tinyllama/Makefile` est encore i386 alors que la base 64‑bit est actée et testée. Cela ajoute de la confusion et complique la CI locale.
- Reco: unifier les builds sur x86‑64 (QEMU `qemu-system-x86_64`) avant d’ajouter Zig.

5) Détails build Zig possiblement non exacts
- `code_model = .kernel` et certaines API Build Zig évoluent selon la version. Il faudra vérifier sur Zig 0.13.x et, au besoin, passer des flags équivalents via `addObject`/`addExecutable` (ex: `-mcmodel=kernel`, `-mno-red-zone`).

6) Chevauchement Zig std vs llvm‑libc
- La stratégie parallèle `docs/architecture/LLVM_LIBC_STRATEGY.md` prévoit migrer `malloc/memcpy/…` vers llvm‑libc pour profiter du JIT. Le plan Zig propose de « remplacer kernel_lib par la std Zig (freestanding) ».
- Reco: clarifier les frontières:
  - Drivers/I/O/CPU: Zig natif (aucun intérêt JIT, code idiomatique low‑level).
  - Mémoire + string: rester côté llvm‑libc (profilable/JITable), exposé à Zig via `@cImport`.
  - Application/Glue: Zig.

7) Viabilité ORC JIT en bare‑metal, court terme
- ORC JIT requiert allocation exécutable, relocations, potentiellement TLS/eh‑frames selon les passes. Tant que paging/permissions ne sont pas en place, cibler un « IR interpreter » ou un JIT minimal hors noyau peut être plus pragmatique.

---

**Ajustements Recommandés Au Plan**
- Phase 1 (Boot Zig minimal) — conserver GRUB/boot.S existant, ajouter `src/kernel.zig` qui:
  - initialise un allocateur Zig `FixedBufferAllocator` sur un buffer BSS aligné;
  - appelle les routines C existantes pour `serial_init()`/`serial_puts()` via `extern`;
  - imprime « BareFlow-Zig hello » et boucle; objectif: image bootable en QEMU x86‑64.
- Build Zig — `build.zig` ciblant `.x86_64, .freestanding, .none`, flags `-mno-red-zone`, `-mcmodel=kernel`, link de `boot.o` (ASM existant) + objets `kernel_lib` nécessaires. Pin `zig 0.13.x` via `build.zig.zon`.
- Mémoire — pas de `page_allocator`; uniquement `FixedBufferAllocator` (tampon statique), avec asserts comptime (taille, alignement, bornes) comme suggéré par le plan.
- Limiter la std — importer `std` avec parcimonie; ne pas utiliser d’API dépendantes d’OS (fichiers, temps mur, env).
- LLVM — remplacer `linkSystemLibrary("LLVM-18")` par une étape ultérieure: archives LLVM statiques cross‑compilées ou fine passerelle C existante; définir un jalon explicite « ORC JIT en kernel Zig » après paging.
- Frontières avec llvm‑libc — décider officiellement: Zig pour drivers/CPU, llvm‑libc pour `malloc/mem*` (profiling/JIT). Documenter l’ABI côté Zig (`extern fn memcpy(...) callconv(.C) ...`).

---

**Étapes Immédiates (1–2 jours)**
- Unifier en x86‑64: corriger `tinyllama/Makefile` pour 64‑bit ou le marquer legacy; continuer à valider sous `tests/phase4/qemu_llvm_64`.
- Ajouter une cible `zig` minimale:
  - `src/kernel.zig` (hello + serial via extern) et buffer BSS pour heap.
  - `build.zig` qui produit `bareflow.elf` et relie `boot.o` existant.
- Critères d’acceptation: boot QEMU x86‑64, message série visible, `alloc/free` Zig sur buffer fonctionnels, BSS réellement zéro (test simple).

**Étapes Suivantes (1–2 semaines)**
- Porter progressivement drivers `serial`, `vga` en Zig; exposer API C actuelle vers Zig pour compat.
- Exposer à Zig les primitives `jit_profile_*` existantes (C) via `extern`.
- Préparer l’intégration llvm‑libc côté Zig (symboles C standards accessibles depuis Zig), mesurer l’empreinte.
- Mettre en place paging + permissions mémoire (pré‑requis ORC JIT en kernel).

---

**Risques & Mitigations**
- Écosystème Zig: versionner l’outil (0.13.x), figer les flags, CI locale avec QEMU.
- Lien LLVM: repousser au jalon « paging prêt »; sinon conserver la voie C actuelle validée.
- Courbe d’apprentissage: démarrer par un noyau Zig minimal viable (hello + serial), itérer par couches.

---

**Conclusion**
Le plan de migration vers Zig est pertinent et bien aligné avec les besoins du projet (sécurité mémoire, simplicité, intégration LLVM à terme). Pour qu’il soit « plug‑and‑play » avec l’état actuel du dépôt, il faut toutefois: (1) unifier l’architecture en 64‑bit partout, (2) démarrer par un noyau Zig minimal adossé au boot/ASM et à `kernel_lib` existants, (3) repousser l’intégration LLVM/ORC au moment où paging/permissions seront en place, et (4) clarifier la frontière Zig vs llvm‑libc. Avec ces ajustements, la migration peut commencer immédiatement et réduire le risque tout en livrant rapidement un premier succès visible en QEMU.

---

**Références Rapides (dépôt)**
- `ZIG_MIGRATION_STRATEGY.md` — plan initial (31 Oct 2025)
- `tests/phase4/qemu_llvm_64/*` — kernel 64‑bit GRUB/Multiboot2 validé
- `kernel_lib/Makefile.llvm` — build 64‑bit runtime (bump allocator)
- `tinyllama/Makefile` — encore i386 (à unifier)

