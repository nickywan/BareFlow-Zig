# BareFlow JIT - Quick Start Guide

Guide rapide pour compiler et tester l'interface JIT avec LLVM 18.

## Prérequis

### Ubuntu/Debian
```bash
# Installer LLVM 18 et les outils de développement
sudo apt update
sudo apt install -y llvm-18-dev llvm-18 clang-18 llvm-18-tools

# Vérifier l'installation
clang-18 --version    # Devrait afficher 18.x.x
llvm-config-18 --version
```

### Vérification rapide
```bash
# Ces commandes doivent fonctionner
which clang-18
which clang++-18
which llvm-link-18
which llvm-dis-18
```

## Compilation

### 1. Compiler la bibliothèque minimal.bc

```bash
# Compiler juste la bibliothèque bitcode
make -f Makefile.jit libs

# Vérifier
ls -lh libs/minimal.bc
# Devrait afficher: libs/minimal.bc (4-5 KB)
```

La bibliothèque contient :
- `strlen()` - Calcul de longueur de chaîne
- `memcpy()` - Copie de mémoire
- `memset()` - Remplissage de mémoire
- `strcpy()` - Copie de chaîne

### 2. Compiler tous les tests

```bash
# Tout compiler (libs + tests)
make -f Makefile.jit all
```

Cela crée :
- `bin/test_jit` - Test basique LLVM LLJIT
- `bin/test_jit_interface` - Test complet de l'interface JIT avec profiling

### 3. Voir la configuration

```bash
make -f Makefile.jit info
```

Output attendu :
```
=== BareFlow JIT Build Configuration ===
CXX: clang++-18
LLVM Config: llvm-config-18
LLVM Version: 18.x.x
LLVM CXXFLAGS: -I/usr/lib/llvm-18/include ...
```

## Exécution des tests

### Test basique

```bash
make -f Makefile.jit test-basic
```

Output attendu :
```
=== Fluid LLVM JIT Test ===

[OK] LLVM JIT created
[OK] Loaded minimal.bc (4 functions)
[OK] Module added to JIT
[OK] Found strlen function

[TEST] strlen("Fluid JIT is ALIVE!") = 19
[EXPECTED] 19

✅ SUCCESS! LLVM JIT works!
```

### Test interface complète

```bash
make -f Makefile.jit test-interface
```

Output attendu :
```
=== BareFlow JIT Interface Test (LLVM 18) ===

[1] Creating JIT context...
    [OK] JIT context created

[2] Loading bitcode module...
    [OK] Module loaded

[3] Looking up 'strlen' function...
    [OK] Function found at 0x...

[4] Testing function with profiling...
    strlen("BareFlow LLVM JIT") = 17
    [OPTIMIZE] Function reoptimized after 100 calls
    [OK] Executed function 150 times

[5] Function profiling info:
    Name: strlen
    Code ptr: 0x...
    Call count: 150
    Total cycles: 150000
    Avg cycles/call: 1000
    Opt level: BASIC (-O1)

[6] Listing all JIT functions:
    Found 1 function(s)
    - strlen (calls: 150)

[7] Global JIT statistics:
    Functions compiled: 1
    Total function calls: 150
    Reoptimizations: 1
    Memory used: 0 bytes

[8] Cleaning up...
    [OK] Cleanup complete

=== ALL TESTS PASSED ===
```

## Fonctionnalités testées

### ✅ Test interface (test_jit_interface)
- [x] Création/destruction du contexte JIT
- [x] Chargement de module bitcode
- [x] Recherche de fonction
- [x] Exécution de code JIT
- [x] **Profiling par fonction** (compteur d'appels, cycles)
- [x] **Auto-optimisation** après 100 appels (O0 → O1)
- [x] **Auto-optimisation** après 1000 appels (O1 → O2/O3)
- [x] Statistiques globales et par fonction
- [x] Listage des fonctions chargées

## Développement

### Ajouter une fonction à minimal.bc

1. Créer `libs/minimal/mafonction.c` :
```c
#include <stddef.h>

int ma_fonction(int x) {
    return x * 2;
}
```

2. Recompiler :
```bash
make -f Makefile.jit clean
make -f Makefile.jit libs
```

3. Tester dans votre code :
```cpp
typedef int (*MaFunc)(int);
MaFunc func = (MaFunc)jit_find_function(ctx, "ma_fonction");
int result = func(21);  // result = 42
```

### Inspecter le bitcode

```bash
# Voir le bitcode LLVM IR
llvm-dis-18 libs/minimal.bc -o -

# Voir les symboles
llvm-nm-18 libs/minimal.bc
```

### Nettoyer

```bash
# Supprimer tous les fichiers générés
make -f Makefile.jit clean
```

## Débogage

### Problème : `llvm-config: error: missing libLLVM...`

**Solution** : Installer `llvm-18-dev` (pas juste `llvm-18`)
```bash
sudo apt install llvm-18-dev
```

### Problème : `fatal error: 'llvm/...h' file not found`

**Cause** : Headers LLVM 18 manquants

**Solution** :
```bash
sudo apt install llvm-18-dev
# Vérifier
ls /usr/lib/llvm-18/include/llvm/
```

### Problème : `libs/minimal.bc: No such file`

**Solution** : Compiler la bibliothèque d'abord
```bash
make -f Makefile.jit libs
```

### Problème : Tests échouent

**Debug étape par étape** :
```bash
# 1. Vérifier LLVM
llvm-config-18 --version

# 2. Compiler juste la lib
make -f Makefile.jit clean
make -f Makefile.jit libs

# 3. Vérifier le bitcode
llvm-dis-18 libs/minimal.bc -o - | grep "define.*strlen"

# 4. Compiler un test
make -f Makefile.jit bin/test_jit_interface

# 5. Exécuter avec verbose
./bin/test_jit_interface
```

## Intégration CI/CD

```yaml
# Exemple GitHub Actions
- name: Install LLVM 18
  run: |
    sudo apt update
    sudo apt install -y llvm-18-dev clang-18

- name: Build and test JIT
  run: |
    make -f Makefile.jit all
    make -f Makefile.jit test
```

## Performance

Optimisations attendues avec le profiling :

| Appels | Niveau opt | Speedup typique |
|--------|-----------|-----------------|
| 0-99   | O0        | 1x (baseline)   |
| 100-999| O1        | 2-3x            |
| 1000+  | O2/O3     | 5-10x           |

Les temps de cycle dans le test sont simulés (fake), mais en production avec `rdtsc()` réel, vous verrez les vraies améliorations.

## Architecture

```
BareFlow-LLVM/
├── kernel/
│   ├── jit_interface.h    ← Interface C pure
│   └── jit_llvm18.cpp     ← Implémentation LLVM 18
├── libs/
│   ├── minimal/           ← Sources C de la bibliothèque
│   │   ├── strlen.c
│   │   ├── memcpy.c
│   │   ├── memset.c
│   │   └── strcpy.c
│   └── minimal.bc         ← Bibliothèque bitcode linkée (généré)
├── test_jit.cpp           ← Test basique
├── test_jit_interface.cpp ← Test complet avec profiling
├── Makefile.jit           ← Build system
└── bin/                   ← Exécutables (généré)
    ├── test_jit
    ├── test_jit_interface
    └── *.bc               ← Bitcode intermédiaire
```

## Ressources

- [LLVM 18 Documentation](https://llvm.org/docs/)
- [LLVM ORC JIT Guide](https://llvm.org/docs/ORCv2.html)
- [JIT_MERGE_README.md](./JIT_MERGE_README.md) - Documentation complète

---

**Ready to go!** 🚀

Si tous les tests passent, votre environnement JIT est opérationnel et prêt pour l'intégration bare-metal.
