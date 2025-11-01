# BareFlow – contexte partagé (à lire avant d’agir)

> Fichier commun à TOUS les agents. Il doit rester COURT.
> Résume seulement ce qui vient d’être décidé, migré, ou est en cours.

## 2025-11-01 – project-overseer
- Décision : kernel désormais **en Zig** (fin des nouveaux ajouts kernel en C).
- Décision : intégration progressive **llvm-libc** (malloc/memcpy/memset en priorité).
- Décision : cibles **x86_64** et **aarch64** (Raspberry Pi 4).
- Règle : chaque agent met à jour ce fichier + son mémo après une action complexe.

## 2025-11-01 – zig-llvm-libc-integrator
- Analyse : fonctions standard en **IR** pour optimisation JIT (profilage/vectorisation).
- TODO : brancher malloc llvm-libc sur le heap du kernel Zig.

## 2025-11-01 – llvm-orc-jit-specialist
- Rappel : tiered JIT O0→O3, hot paths, auto-vectorisation selon CPU features.
- TODO : format de cache persistant incluant (arch, isa), vérif reload sécurisée.
