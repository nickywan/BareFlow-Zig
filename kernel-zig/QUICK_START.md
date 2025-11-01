# BareFlow Zig - Quick Start

## État Actuel ✅

Tous les problèmes de boot sont résolus:
- ✅ ISO BIOS-compatible créé
- ✅ Kernel sans relocations (required by GRUB)
- ✅ Page tables dans .data (pas écrasées par BSS)
- ✅ Multiboot2 header validé

## Test Rapide

### 1. Kernel 32-bit Simple (Test GRUB/Multiboot)
```bash
qemu-system-x86_64 -machine type=pc -m 128 -cdrom test32-static.iso
```
**Attendu**: Affiche "32OK!" sur l'écran VGA

### 2. Kernel Zig Complet (64-bit avec tests)
```bash
qemu-system-x86_64 -machine type=pc -m 128 -cdrom bareflow-zig-final.iso
```
**Attendu**: 
- Markers '1234' pendant boot (boot.S)
- Output du kernel (VGA ou serial)

### 3. Avec Serial Debug
```bash
qemu-system-x86_64 -machine type=pc -m 128 \
  -cdrom bareflow-zig-final.iso \
  -serial file:serial.log

# Vérifier la sortie
cat serial.log
```

## Build

```bash
# Rebuild kernel
export PATH="/tmp/zig-linux-x86_64-0.13.0:/usr/bin:/bin:$PATH"
zig build

# Create ISO
cp .zig-cache/o/*/kernel iso/boot/kernel
grub-mkrescue -d /usr/lib/grub/i386-pc -o bareflow-zig-final.iso iso/
```

## Problèmes Résolus Cette Session

1. **ISO EFI ne bootait pas** → Utilisé `-d /usr/lib/grub/i386-pc`
2. **Page tables écrasées** → Déplacées dans .data
3. **GRUB "elf file with reloc"** → Ajouté `kernel.pie = false` dans build.zig

Voir `BOOT_DEBUG_STATUS.md` pour détails complets.
