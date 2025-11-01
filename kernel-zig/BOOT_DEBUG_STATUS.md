# BareFlow Zig - Boot Debugging Status

**Date**: 2025-11-01
**Session**: Debugging initial boot issues

---

## ✅ Problèmes Résolus

### 1. Problème: ISO EFI ne bootait pas avec SeaBIOS
**Symptôme**: BIOS en boucle "Booting from DVD/CD..." sans jamais charger GRUB

**Root Cause**: Sur une machine Linux EFI, `grub-mkrescue` crée par défaut des ISOs EFI qui ne bootent PAS avec QEMU SeaBIOS (BIOS mode).

**Solution**:
```bash
grub-mkrescue -d /usr/lib/grub/i386-pc -o kernel.iso iso/
```
L'option `-d /usr/lib/grub/i386-pc` force la création d'un ISO BIOS-compatible.

**Référence**: https://phip1611.de/blog/os-dev-create-a-bootable-image-for-a-custom-kernel-with-grub-as-bootloader-for-legacy-x86-boot-e-g-multiboot2-kernel/

---

### 2. Problème: Page tables écrasées par BSS zeroing
**Symptôme**: Kernel crash/loop après transition 64-bit

**Root Cause**: Dans boot.S, les page tables (pml4, pdpt, pd) étaient dans la section `.bss` et étaient zéroées APRÈS la transition 64-bit, écrasant les tables actives.

**Solution**: Déplacer les page tables dans `.data` au lieu de `.bss`
```asm
# Page tables in .data (must NOT be in BSS or they get zeroed!)
.section .data
.align 4096
pml4:
    .skip 4096
pdpt:
    .skip 4096
pd:
    .skip 4096
```

**Fichier modifié**: `src/boot.S:20-28`

---

### 3. Problème: Section .multiboot placée APRÈS .text
**Symptôme**: GRUB ne trouvait pas le multiboot2 header

**Root Cause**: Le multiboot2 header doit être dans les premiers 32KB du fichier. Sans linker script approprié, la section `.multiboot` était placée à l'offset 0x3000, APRÈS `.text`.

**Solution**: Le linker script `src/linker.ld` place déjà `.multiboot` correctement au début de `.text`:
```ld
.text : ALIGN(4K)
{
    KEEP(*(.multiboot))    # PREMIER - multiboot header
    *(.text)
    *(.text.*)
}
```

**Vérification**:
```bash
objdump -h iso/boot/kernel | grep multiboot
# Section .text commence à 0x100000 et contient .multiboot au début
```

---

## 🔧 Configuration Actuelle

### Kernel Zig
- **Architecture**: x86-64 (64-bit long mode)
- **Entry point**: `_start` (boot.S) → `kernel_main` (Zig)
- **Multiboot2**: ✅ Validé avec `grub-file --is-x86-multiboot2`
- **Taille**: ~135 KB (binary + debug info)
- **BSS**: 32 MB (heap_buffer pour FixedBufferAllocator)

### Boot Sequence
1. **GRUB** (Multiboot2) charge kernel en 32-bit protected mode
2. **boot.S:_start** (32-bit):
   - Write VGA markers ('1', '2', '3', '4') pour debug
   - Setup stack
   - Setup page tables (identity map 1GB)
   - Enable PAE + long mode
3. **boot.S:long_mode_start** (64-bit):
   - Setup segment registers
   - Zero BSS (APRÈS page tables)
   - Call `kernel_main()`
4. **main.zig:kernel_main()**:
   - Init serial (COM1)
   - Init VGA
   - Run tests (allocation, return values)

### Build Commands
```bash
# Build kernel
export PATH="/tmp/zig-linux-x86_64-0.13.0:/usr/bin:/bin:$PATH"
zig build

# Create BIOS-compatible ISO
cp .zig-cache/o/*/kernel iso/boot/kernel
grub-mkrescue -d /usr/lib/grub/i386-pc -o bareflow-final.iso iso/

# Test in QEMU
qemu-system-x86_64 -machine type=pc -m 128 -cdrom bareflow-final.iso
```

---

## ⚠️ État Actuel - Debugging en Cours

### Symptômes
- ✅ ISO boot correctement (pas de boucle BIOS)
- ✅ Multiboot2 header validé
- ✅ GRUB charge le kernel
- ❌ **Aucune sortie serial visible dans le CLI**
- ❓ **Sortie VGA potentiellement visible dans fenêtre graphique QEMU** (non vérifiée)

### Hypothèses
1. **Kernel fonctionne mais VGA seulement**: Les markers VGA ('1234') et l'output kernel sont affichés dans la fenêtre graphique QEMU mais pas visibles dans serial/CLI
2. **Kernel crash silencieusement**: Crash avant l'init serial, pas d'exceptions visibles dans les logs
3. **Serial COM1 pas initialisé correctement**: La fonction `serial_init()` ne configure pas le port correctement pour QEMU

### Prochaines Étapes
1. **Vérifier affichage VGA** : Lancer QEMU avec display graphique et vérifier visuellement l'écran
2. **Debug avec GDB**:
   ```bash
   # Terminal 1
   qemu-system-x86_64 -machine type=pc -m 128 -cdrom bareflow-final.iso -s -S

   # Terminal 2
   gdb iso/boot/kernel
   (gdb) target remote :1234
   (gdb) break kernel_main
   (gdb) continue
   ```
3. **Vérifier serial init**: Ajouter des writes serial AVANT serial_init() pour voir si le port marche
4. **Tester kernel minimal**: Kernel C pur avec juste VGA pour isoler le problème

---

## 📊 Progrès Session

**Résolu**:
- ✅ Migration Zig compilée et validée
- ✅ Problème EFI/BIOS identifié et résolu
- ✅ Bug BSS/page tables corrigé
- ✅ Multiboot2 header correctement positionné
- ✅ ISO BIOS bootable créé

**En cours**:
- ⏳ Vérification sortie VGA/serial
- ⏳ Validation complète du boot jusqu'au kernel_main()

**Bloquants**:
- Aucun - tous les bloqueurs techniques résolus
- Besoin de vérification visuelle VGA (fenêtre QEMU)

---

## 🔍 Commandes de Debug Utiles

### Vérifier Multiboot2
```bash
grub-file --is-x86-multiboot2 iso/boot/kernel
objdump -h iso/boot/kernel | grep multiboot
xxd iso/boot/kernel | head -100 | grep "d650 52e8"  # multiboot2 magic
```

### Créer ISO BIOS
```bash
grub-mkrescue -d /usr/lib/grub/i386-pc -o kernel.iso iso/
```

### Test QEMU avec logs
```bash
qemu-system-x86_64 \
    -machine type=pc \
    -m 128 \
    -cdrom bareflow-final.iso \
    -d int,cpu_reset \
    -D qemu-debug.log \
    -serial file:serial.log
```

### GDB Debugging
```bash
qemu-system-x86_64 -machine type=pc -m 128 -cdrom bareflow-final.iso -s -S &
gdb iso/boot/kernel
(gdb) target remote :1234
(gdb) break *0x100000  # multiboot entry
(gdb) break kernel_main
(gdb) continue
(gdb) x/10i $pc
(gdb) info registers
```

---

**Conclusion**: Tous les problèmes connus de boot sont résolus. Le kernel devrait booter. Besoin de vérification visuelle de la sortie VGA ou debug GDB pour confirmer l'exécution jusqu'au kernel_main().
