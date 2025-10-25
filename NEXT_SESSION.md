# 🚀 Guide de Démarrage - Prochaine Session

## ⚡ Quick Start

```bash
# 1. Commiter le travail de la session 7
./commit_session7.sh

# 2. Pousser les commits
git push origin $(git branch --show-current)

# 3. Lancer un test complet
INTERACTIVE=1 make clean && INTERACTIVE=1 make all
./run_fat16_interactive.sh

# 4. Vérifier que tout fonctionne
# - JIT allocator: 10/10 tests ✓
# - FAT16: Init + list + read ✓
# - Modules: 9 modules executés ✓
```

## 📋 État Actuel (Session 7 - Fin)

### ✅ Complété
- FAT16 driver complet et testé
- Multi-iteration PGO workflow
- Per-function profiling system
- JIT allocator optimisé (896KB pools)
- Documentation complète

### 📦 Fichiers Prêts à Commiter
Tous dans le répertoire de travail, exécutez:
```bash
./commit_session7.sh
```

### 🎯 Prochain Objectif: Load Modules from Disk

**Goal**: Charger les modules .MOD depuis le disque FAT16 au lieu du cache embarqué

**Steps**:
1. Modifier `kernel/module_loader.c`:
   - Ajouter fonction `module_load_from_disk(fs, filename)`
   - Intégrer avec FAT16 driver

2. Créer `kernel/disk_module_loader.c`:
   - Wrapper pour FAT16 + module loading
   - API: `disk_load_module(fs, name)`

3. Modifier `kernel/kernel.c`:
   - Initialiser FAT16 au boot
   - Tester chargement d'un module depuis disque
   - Fallback sur embedded si disque absent

4. Tester avec module simple:
   ```bash
   # Copier un module sur disque FAT16
   mcopy -i build/fat16_test.img modules/sum.mod ::SUM.MOD

   # Lancer et charger depuis disque
   ./run_fat16_interactive.sh
   ```

## 🔨 Commandes Utiles

### Build & Test
```bash
# Build clean
make clean && INTERACTIVE=1 make all

# Test interactif avec FAT16
./run_fat16_interactive.sh

# PGO multi-iteration
python3 tools/pgo_multi_iteration.py

# Créer disque FAT16
python3 tools/create_fat16_disk.py
```

### Disk Operations
```bash
# Lister fichiers FAT16
mdir -i build/fat16_test.img ::

# Copier fichier sur disque
mcopy -i build/fat16_test.img file.txt ::FILE.TXT

# Lire fichier depuis disque
mcopy -i build/fat16_test.img ::TEST.TXT - | cat

# Info filesystem
minfo -i build/fat16_test.img
```

### Git Operations
```bash
# Commiter session 7
./commit_session7.sh

# Review commits
git log --oneline -10
git log --stat -6

# Push
git push origin $(git branch --show-current)

# Nouveau commit
git add <files>
git commit -m "feat(...): ...

🤖 Generated with Claude Code
Co-Authored-By: Claude <noreply@anthropic.com>"
```

## 📝 Template: Module Loader from Disk

### Pseudocode
```c
// kernel/disk_module_loader.h
int disk_load_module(fat16_fs_t* fs, const char* filename,
                     void** code_ptr, uint32_t* code_size);

// Usage in kernel.c
fat16_fs_t fs;
if (fat16_init(&fs, 1, 0) == 0) {
    void* code;
    uint32_t size;

    if (disk_load_module(&fs, "SUM.MOD", &code, &size) == 0) {
        module_info_t module;
        if (module_load(&mgr, code, size, &module) == 0) {
            // Execute module
            int result = module_execute(&mgr, "sum");
        }
    }
}
```

### Implementation Steps
1. **Read module from disk**:
   - Use `fat16_open()` + `fat16_read()`
   - Allocate buffer with `malloc()`
   - Copy file content to buffer

2. **Validate module**:
   - Check magic bytes
   - Verify size matches

3. **Load with existing system**:
   - Use `module_load()` from module_loader.c
   - Pass loaded code pointer

4. **Memory management**:
   - Keep code in memory (don't free)
   - Or implement module unload

## 🐛 Troubleshooting

### FAT16 Issues
```bash
# Si "invalid bytes per sector"
# → Vérifier que le disque est bien formaté
file build/fat16_test.img
minfo -i build/fat16_test.img

# Si "initialization failed"
# → Recréer le disque
python3 tools/create_fat16_disk.py

# Si fichier non trouvé
# → Vérifier avec mdir
mdir -i build/fat16_test.img ::
```

### JIT Allocator Issues
```bash
# Si "allocation failed"
# → Augmenter RAM dans script
# Modifier run_fat16_interactive.sh: -m 64M → -m 128M

# Ou augmenter pool sizes
# Modifier kernel/jit_allocator_test.c:
# jit_allocator_init(512*1024, 1024*1024, 256*1024)
```

### Build Issues
```bash
# Si erreurs de linking
make clean && make all

# Si kernel trop gros
ls -lh build/kernel.bin
# Max: ~64KB avec 128 sectors

# Si bootloader échoue
# → Augmenter KERNEL_SECTORS dans boot/stage2.asm
```

## 📚 Documentation

- **FAT16_TEST_GUIDE.md**: Guide complet FAT16
- **AUTONOMOUS_WORKFLOW.md**: Workflow autonome
- **CLAUDE_CONTEXT.md**: Contexte complet
- **ROADMAP.md**: Roadmap projet

## 🎯 Milestone: Disk Module Loading

**Definition of Done**:
- [ ] Fonction `disk_load_module()` implémentée
- [ ] Au moins 1 module chargé depuis disque
- [ ] Test interactif validé
- [ ] Fallback sur embedded si disque absent
- [ ] Performance comparée (disk vs embedded)
- [ ] Documentation mise à jour
- [ ] Commit avec message détaillé

**Estimated Effort**: 2-3 heures

**Blockers**: Aucun, tout est prêt!

---

**Ready to Start**: ✅ Tous les outils sont en place!
