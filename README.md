# 🔥 Fluid OS - Self-Optimizing Kernel

**Un unikernel avec LLVM JIT runtime qui s'optimise lui-même à chaud.**

## Quick Start

### Phase 1 (User-Space JIT) - DONE ✅
```bash
# Build module
clang-18 -c -emit-llvm -O2 -ffreestanding -nostdlib \
    kernel/stdlib.c -o kernel/stdlib.bc
clang-18 -c -emit-llvm -O2 -ffreestanding -nostdlib \
    app/test.c -o app/test.bc
llvm-link-14 kernel/stdlib.bc app/test.bc -o fluid_module.bc

# Test JIT
cd jit_test
./jit_profiler ../fluid_module.bc
```

### Phase 2 (Kernel Bare Metal) - IN PROGRESS 🚧
```bash
make
make run
```

## Architecture

```
Application (LLVM IR)
       ↕
Kernel Fluid (Ring 0)
  ├── LLVM ORC JIT
  ├── Profiler Runtime
  └── Re-JIT Optimizer
```

**Innovation**: Kernel qui se JIT lui-même pour s'adapter au workload.

## Boot Process - Two Stage Bootloader

Le système utilise un bootloader en deux étapes pour charger le kernel en mode protégé 32-bit.

### 📋 Disk Layout

```
┌────────────────────────────────────┐
│ Sector 0 (512 bytes)               │ ← Stage 1 Bootloader
├────────────────────────────────────┤
│ Sectors 1-8 (4 KB)                 │ ← Stage 2 Bootloader
├────────────────────────────────────┤
│ Sectors 9+ (variable)              │ ← Kernel Binary
└────────────────────────────────────┘
```

### 🚀 Stage 1 - Initial Bootloader (boot/stage1.asm)

**Taille**: 512 bytes (1 secteur)
**Adresse de chargement**: 0x7C00 (chargé par le BIOS)
**Responsabilités**:

1. **Initialisation basique**
   - Configure les segments (DS, ES, SS = 0x0000)
   - Configure la pile (SP = 0x7C00, pile descendante)
   - Sauvegarde le numéro du disque de boot (DL)

2. **Chargement de Stage 2**
   - Charge 8 secteurs (4 KB) depuis le disque
   - Secteurs 2-9 → adresse mémoire 0x7E00
   - Utilise INT 0x13 (BIOS disk services) en mode CHS
   - 3 tentatives de retry en cas d'erreur

3. **Transfert de contrôle**
   - Passe le numéro de disque à Stage 2 (via DL)
   - Saute vers Stage 2 à l'adresse 0x7E00

**Limitations**:
- Mode réel 16-bit seulement
- Pas de support LBA (seulement CHS pour simplicité)
- Taille fixe de 512 bytes (contrainte BIOS)

### 🔧 Stage 2 - Extended Bootloader (boot/stage2.asm)

**Taille**: 4 KB (8 secteurs)
**Adresse de chargement**: 0x7E00
**Responsabilités**:

#### 1. **Détection du support LBA**
```
INT 0x13, AH=0x41 (Check Extensions Present)
→ Permet de charger le kernel avec LBA (plus moderne) ou CHS (fallback)
```

#### 2. **Activation de la ligne A20**
La ligne A20 permet d'accéder à la mémoire au-delà de 1 MB. Trois méthodes tentées dans l'ordre :

| Méthode | Description | Avantage |
|---------|-------------|----------|
| **BIOS** | INT 0x15, AX=0x2401 | Simple et portable |
| **Keyboard Controller** | Via ports 0x64/0x60 | Compatible avec anciens PC |
| **Fast A20** | Port 0x92 | Rapide sur PC récents |

Vérification avec test mémoire à 0x7E00 vs 0xFFFF:0x7E10.

#### 3. **Chargement du kernel**
```
Secteur 9+ → adresse mémoire 0x1000
Taille: 15 secteurs (~7.5 KB)
Méthode: LBA (préféré) ou CHS (fallback)
Retry: 3 tentatives maximum
```

#### 4. **Vérification de signature**
```
Vérifie que les 4 premiers bytes du kernel = "FLUD" (0x464C5544)
Empêche l'exécution de données corrompues
```

#### 5. **Configuration de la GDT** (Global Descriptor Table)
```
┌─────────────┬────────────────┬──────────────────┐
│ NULL (0x00) │ CODE (0x08)    │ DATA (0x10)      │
└─────────────┴────────────────┴──────────────────┘
       ↓              ↓                  ↓
    Requis      Ring 0, 4GB       Ring 0, 4GB
               Executable         Writable
```

#### 6. **Passage en mode protégé**
```assembly
cli                    ; Désactive les interruptions
lgdt [gdt_descriptor]  ; Charge la GDT
mov cr0, 0x1          ; Active le bit PE (Protection Enable)
jmp CODE_SEG:protected_mode_start  ; Far jump pour flush du pipeline
```

#### 7. **Initialisation mode protégé (32-bit)**
```
- Configure tous les segments (DS, SS, ES, FS, GS) → 0x10 (DATA_SEG)
- Configure la pile à 0x90000 (EBP/ESP)
- Saute vers le kernel à 0x1000
```

### 🎯 Kernel Entry Point (kernel/entry.asm)

**Signature**: `"FLUD"` (0x464C5544) - les 4 premiers bytes
**Adresse**: 0x1000
**Format**: ELF 32-bit flat binary

Le kernel reçoit le contrôle en mode protégé 32-bit avec :
- CS = 0x08 (code segment)
- DS/SS/ES/FS/GS = 0x10 (data segment)
- ESP = 0x90000 (pile configurée)
- Interruptions désactivées (CLI)

Puis `kernel_main()` prend le relais (kernel/kernel.c).

### 🔍 Boot Sequence Summary

```
┌─────────────┐
│    BIOS     │ Charge Sector 0 → 0x7C00
└──────┬──────┘
       ↓
┌─────────────┐
│   Stage 1   │ Charge Sectors 1-8 → 0x7E00
│  (0x7C00)   │ Jump → 0x7E00
└──────┬──────┘
       ↓
┌─────────────┐
│   Stage 2   │ 1. Détecte LBA
│  (0x7E00)   │ 2. Active A20
│             │ 3. Charge Kernel → 0x1000
│             │ 4. Vérifie signature "FLUD"
│             │ 5. Configure GDT
│             │ 6. Mode protégé
│             │ 7. Jump → 0x1000
└──────┬──────┘
       ↓
┌─────────────┐
│   Kernel    │ kernel_main() en Ring 0
│  (0x1000)   │ Mode protégé 32-bit
└─────────────┘
```

### ⚠️ Error Handling

Stage 2 utilise des codes d'erreur en hexadécimal :

| Code | Erreur | Cause probable |
|------|--------|----------------|
| `0xD1` | DISK_READ | Échec lecture disque |
| `0xD2` | DISK_VERIFY | Nombre de secteurs incorrect |
| `0xA2` | A20_FAILED | Ligne A20 non activable |
| `0x51` | BAD_SIGNATURE | Signature kernel invalide |
| `0xE0` | NO_KERNEL | Kernel non trouvé |

En cas d'erreur fatale, le système affiche `FATAL ERROR: 0xXX` et stoppe (CLI/HLT).

## Build System

Le Makefile orchestre la construction complète :

```bash
make         # Build tout
make run     # Build + lancement QEMU
make debug   # Build + QEMU avec logs CPU
make clean   # Nettoyage
make rebuild # Clean + build
```

**Pipeline de build** :
1. Stage 1 (NASM) → `build/stage1.bin` (512 bytes exact)
2. Stage 2 (NASM) → `build/stage2.bin` (4096 bytes exact)
3. Kernel ASM (NASM) + C (GCC) → `build/kernel.bin`
4. Assemblage avec `dd` → `fluid.img` (disquette 1.44 MB)

## Status

- ✅ Phase 1: POC User-Space (VALIDÉE)
- 🚧 Phase 2: Kernel Bare Metal (EN COURS)
  - ✅ Two-stage bootloader
  - ✅ Protected mode
  - ✅ VGA text mode
  - ✅ Memory allocator
  - ⏳ LLVM JIT integration
- ⏳ Phase 3: llama.cpp Integration (FUTUR)

## Documentation

Voir [PROJECT.md](PROJECT.md) pour documentation complète.

## License

Educational/Research - Not production ready
