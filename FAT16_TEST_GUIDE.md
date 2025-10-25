# Guide de Test FAT16 - BareFlow Kernel

## 🔧 Corrections Apportées

### Problème 1: Drive ATA Incorrect
**Symptôme**: `FAT16 invalid bytes per sector`, `initialization failed`

**Cause**: Le code utilisait le drive 0 (master), mais QEMU avec `-drive index=1` place le deuxième disque sur le drive 1 (slave).

**Solution**: Modification du driver pour sélectionner le drive ATA correct:
- Drive 0 (master): `0xE0` dans le registre ATA_DRIVE
- Drive 1 (slave): `0xF0` dans le registre ATA_DRIVE

### Problème 2: Mémoire Insuffisante pour JIT Allocator
**Symptôme**: `Code pool allocation failed`, `test fragmentation failed`

**Cause**: QEMU configuré avec seulement 32MB de RAM

**Solution**: Augmentation à 64MB de RAM dans le script de lancement

## 🚀 Lancer le Test

```bash
./run_fat16_interactive.sh
```

## 📋 Étapes du Test FAT16

Le test s'exécute avec des pauses clavier entre chaque étape:

### 1️⃣ Initialisation
- Affiche: `[1] Initializing FAT16 filesystem on drive 1 (slave)...`
- Résultat attendu: `✓ FAT16 initialized successfully`
- **⏸️ Pause**: Appuyez sur une touche

### 2️⃣ Informations du Filesystem
- Affiche: `[2] Filesystem Information:`
  - Bytes per sector: 512
  - Sectors per cluster: 8
  - Reserved sectors: 8
  - Number of FATs: 2
  - Root entries: 512
  - Sectors per FAT: 16
- **⏸️ Pause**: Appuyez sur une touche

### 3️⃣ Listage des Fichiers
- Affiche: `[3] Listing files in root directory:`
- Devrait lister 18 fichiers:
  - TEST.TXT
  - README.TXT
  - 16 modules .MOD (FIBONACCI, SUM, COMPUTE, etc.)
- **⏸️ Pause**: Appuyez sur une touche

### 4️⃣ Lecture de Fichier
- Affiche: `[4] Testing file read (TEST.TXT):`
- Devrait afficher:
  - `✓ File found: TEST.TXT`
  - `Size: 70 bytes`
  - `Content: Hello from FAT16 filesystem!`
  - `This file was created by BareFlow tools.`
- **⏸️ Pause**: Appuyez sur une touche

### 5️⃣ Fin du Test
- Affiche: `✓ FAT16 test complete!`
- Continue avec les tests de modules

## 📦 Contenu du Disque FAT16

Le disque `build/fat16_test.img` (16 MB, FAT16) contient:

### Fichiers de Test
- `TEST.TXT` (70 bytes) - Message de test
- `README.TXT` (199 bytes) - Documentation

### Modules Compilés (16 fichiers)
- `COMPUTE.MOD` / `COMPUTE_O3.MOD`
- `FFT_1D.MOD` / `FFT_1D_O1.MOD`
- `FIBONACCI.MOD` / `FIBONACCI_O1.MOD`
- `MATRIX_MUL.MOD` / `MATRIX_MUL_O1.MOD`
- `PRIMES.MOD` / `PRIMES_O2.MOD`
- `QUICKSORT.MOD`
- `SHA256.MOD` / `SHA256_O1.MOD`
- `STROPS.MOD`
- `SUM.MOD` / `SUM_O1.MOD`

## 🔍 Vérification du Disque

Pour vérifier le contenu du disque FAT16 depuis l'hôte:

```bash
# Lister les fichiers
mdir -i build/fat16_test.img ::

# Voir le contenu d'un fichier
mcopy -i build/fat16_test.img ::TEST.TXT - | cat

# Afficher les informations du filesystem
minfo -i build/fat16_test.img
```

## ⚙️ Configuration QEMU

Le script `run_fat16_interactive.sh` lance QEMU avec:
- **Drive 0**: `fluid.img` (disque de boot avec kernel)
- **Drive 1**: `build/fat16_test.img` (disque FAT16 en lecture seule)
- **RAM**: 64 MB (suffisant pour JIT allocator + tests)

## 🐛 Dépannage

### Si le test FAT16 échoue toujours

1. **Vérifier que le disque existe**:
   ```bash
   ls -lh build/fat16_test.img
   ```

2. **Vérifier le format FAT16**:
   ```bash
   file build/fat16_test.img
   minfo -i build/fat16_test.img
   ```

3. **Recréer le disque**:
   ```bash
   python3 tools/create_fat16_disk.py
   ```

### Si les erreurs JIT persistent

Le JIT allocator peut nécessiter plus de mémoire. Modifiez `-m 64M` en `-m 128M` dans le script si nécessaire.

## ✅ Résultat Attendu

Si tout fonctionne correctement, vous devriez voir:
- ✅ Tous les tests C++ runtime passent
- ✅ Tous les tests JIT allocator passent (10/10)
- ✅ FAT16 s'initialise avec succès
- ✅ 18 fichiers listés dans le répertoire racine
- ✅ Contenu de TEST.TXT affiché
- ✅ Tous les modules s'exécutent sans erreur

## 📝 Notes

- Le driver FAT16 est **read-only** (pas d'écriture)
- Il utilise l'accès direct aux ports ATA/IDE (pas de BIOS)
- Le support LBA 28-bit permet des disques jusqu'à 128 GB
- Le format 8.3 (DOS) est requis pour les noms de fichiers
