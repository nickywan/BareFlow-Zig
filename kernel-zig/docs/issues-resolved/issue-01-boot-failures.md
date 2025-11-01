# Issue #1: Kernel ne boot pas - CPU reset immédiat

**Catégorie**: Boot Issues
**Sévérité**: 🔴 CRITIQUE
**Sessions**: 46, 47

---

## Symptômes

- QEMU affiche "Booting from CD-ROM" puis reset immédiat
- Aucune sortie série
- CPU triple fault ou exception non gérée

---

## Causes et Solutions

### 1.1 Multiboot2 Header Invalide

**Problème**: GRUB ne reconnaît pas le kernel comme bootable

**Solution**:
```assembly
# boot64.S - Section .multiboot
.section .multiboot
.align 8
multiboot_header_start:
    .long 0xE85250D6                    # Magic number (Multiboot2)
    .long 0                             # Architecture: i386 (protected mode)
    .long multiboot_header_end - multiboot_header_start
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))
```

**Validation**:
```bash
grub-file --is-x86-multiboot2 iso/boot/kernel
echo $?  # Doit retourner 0
```

---

### 1.2 BSS Non-Zeroed (Pages Tables Corrompues)

**Problème**: Page tables dans .bss contiennent des données aléatoires
**Symptôme**: Triple fault lors de l'activation de la pagination

**Solution**: Zeroer le BSS AVANT d'activer la pagination

```assembly
# boot64.S - AVANT cr0/cr3 setup
# Zero BSS section
mov $__bss_start, %rdi
mov $__bss_end, %rcx
sub %rdi, %rcx
xor %rax, %rax
rep stosb
```

**Référence**: Session 46, commit 1d01b7c

---

### 1.3 Mismatch 32-bit Boot / 64-bit Kernel

**Problème**: boot.S (32-bit) essaie de charger kernel 64-bit
**Symptôme**: Kernel ne démarre jamais après "Booting from CD-ROM"

**Avant (CASSÉ)**:
```assembly
# boot.S - 32-bit bootloader
.code32
# ... setup page tables
# ... passage en long mode
jmp kernel_main  # ❌ Kernel 64-bit, incompatible
```

**Après (FONCTIONNE)**:
```assembly
# boot64.S - Native 64-bit
.code64
kernel_entry:
    # GRUB nous met déjà en 64-bit!
    mov $stack_top, %rsp
    call kernel_main  # ✅ Compatible
```

**Référence**: Session 47, commit f34f03d - BREAKTHROUGH

---

### 1.4 Stack Overflow / Red Zone Corruption

**Problème**: Stack corrompu par interruptions
**Symptôme**: Crash aléatoire, données corrompues

**Solution**: Désactiver red zone + stack 8MB

```bash
# Compilation flags
-mno-red-zone      # CRITIQUE pour kernel 64-bit
-mcmodel=kernel    # Adressage 64-bit
```

```zig
// build.zig
kernel.root_module.red_zone = false;
```

**Référence**: Session 46, 47

---

## Validation Complète

```bash
# 1. Vérifier Multiboot2
grub-file --is-x86-multiboot2 iso/boot/kernel
echo $?  # Doit être 0

# 2. Vérifier type ELF
readelf -h iso/boot/kernel | grep Type
# Doit être: EXEC (pas DYN)

# 3. Vérifier sections
objdump -h iso/boot/kernel | grep -E "\.text|\.rodata|\.data|\.bss"

# 4. Vérifier entry point
readelf -h iso/boot/kernel | grep Entry

# 5. Test boot
timeout 3 qemu-system-x86_64 -M q35 -m 128 -cdrom kernel.iso \
    -serial file:serial.log
cat serial.log
```

---

## Fichiers Concernés

- `src/boot64.S` - Native 64-bit boot code
- `src/linker.ld` - BSS boundaries definition
- `build.zig` - Compilation flags
