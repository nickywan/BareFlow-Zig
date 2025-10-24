# Revue Codex – Branche `claude/merge-interface-runtime-011CUMDiW4omhPaJemQSVuoR`

## Compréhension du projet
- Vision confirmée : unikernel bare-metal focalisé sur un seul programme/module, exécuté en ring 0 sans changement de contexte ni multitâche. L’objectif est de disposer d’un kernel auto-optimisable à la volée via LLVM JIT, comparable à une JVM dédiée au C/C++ mais allégée pour l’embarqué.
- Phase actuelle : jalon 1.2 (`Module System Improvements`) selon `ROADMAP.md`, avec priorités sur le chargement dynamique, le support LLVM bitcode et l’enrichissement du profiling.
- Livrables déjà visibles : bootloader bi-étage, profilage cycle-accurat, gestion mémoire de base, interface LLJIT côté userspace, squelette d’allocateur JIT et runtime C++ minimal.

## Constatations majeures (revue de code)
- `kernel/jit_allocator.c:143-151` – Les allocations alignées sautent un bloc mal aligné sans ajustement, ce qui rend impossible toute allocation stricte sur un pool fraîchement initialisé. À corriger via alignement différé ou découpe du bloc.
- `kernel/cxx_runtime.cpp:249-255` – Les destructeurs globaux sont invoqués dans l’ordre croissant ; il faut inverser pour respecter l’ABI Itanium et éviter des dépendances mal résolues entre objets statiques.
- `kernel/jit_llvm18.cpp:259-277` – Le chemin de réoptimisation renvoie 0 après recompilation. Les appels attendent un retour positif pour signaler une action, ce qui masque le comportement voulu.

## Questions ouvertes
1. Souhaitez-vous qu’on introduise dès maintenant une stratégie de réalignement dans l’allocateur (padding en tête de bloc) ou préférez-vous un ajustement simple du pointeur de base lors de l’initialisation du pool ?
2. Quelle est la cible minimale de fonctionnalités pour valider la tâche 1.2 : chargement binaire simple depuis disque, ou faut-il déjà intégrer le pipeline LLVM bitcode complet ?
3. Avez-vous une contrainte de taille mémoire pour `fluid.img` qui doit orienter les tailles par défaut des pools JIT et des modules embarqués ?

## Avis sur la faisabilité
Le projet est ambitieux mais cohérent : LLVM 18 offre toutes les briques nécessaires pour un JIT bare-metal, et les jalons graduels du roadmap (profiling -> chargement dynamique -> cache persistant) décomposent bien la complexité. Les principaux risques résident dans :
- l’intégration basse couche de LLVM (gestion de la mémoire exécutable, relocations, dépendances libc++),
- le support disque/FAT pour charger des modules JIT en 1.2,
- la maîtrise de la consommation mémoire pour héberger TinyLlama et son cache.

Avec une progression incrémentale et des tests QEMU systématiques, la faisabilité reste élevée, sous réserve de réserver du temps pour l’adaptation de LLVM aux contraintes freestanding (allocateur dédié, stub des dépendances POSIX). Des retours réguliers sur les jalons permettront d’anticiper les points durs (notamment la persistance du cache et la compilation croisée de TinyLlama).***
