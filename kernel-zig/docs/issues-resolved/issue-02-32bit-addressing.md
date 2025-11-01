# Issue #2: Zig génère des adresses 32-bit au lieu de 64-bit

**Catégorie**: Compilation Issues
**Sévérité**: 🔴 CRITIQUE
**Session**: 47 Part 2

---

## Symptômes

- Infinite 'E' characters sur sortie série
- Garbage data au lieu des strings attendus
- Kernel semble "boucler" sans raison

---

## Diagnostic

```bash
objdump -d iso/boot/kernel | grep -A5 kernel_main
```

**Problème Identifié**:
```assembly
# ❌ CASSÉ - Zig génère EDI (32-bit)
mov $0x101011,%edi    # Charge adresse dans registre 32-bit
call serial_print     # Pointeur invalide!

# ✅ CORRECT - Devrait être RDI (64-bit)
mov $0x101011,%rdi    # Charge adresse dans registre 64-bit
```

---

## Cause Racine

Flag `-mcmodel=kernel` seulement sur boot64.S, pas sur code Zig.

**User Insight**: *"Quand je te parlais de regler ce proble c'est les pointeurs 32 bits"*

---

## Solution

```bash
# Compilation manuelle avec code model kernel
zig build-obj -target x86_64-freestanding -mcmodel=kernel \
    -O ReleaseSafe src/main_simple.zig -femit-bin=main_simple.o
```

**Dans build.zig** (documentation):
```zig
// NOTE (Session 47): kernel.root_module.code_model property does not exist in Zig 0.13
// Manual compilation required:
//   zig build-obj -mcmodel=kernel src/main_simple.zig
```

---

## Résultat - Bonus Optimization

Avec `-mcmodel=kernel`, Zig inline complètement `serial_print()`:

```assembly
# Plus de pointeurs du tout! Sortie directe des caractères
mov $0x42,%al   # 'B'
out %al,(%dx)
mov $0x61,%al   # 'a'
out %al,(%dx)
```

---

## Validation

```bash
# 1. Vérifier assembly généré
objdump -d iso/boot/kernel | grep -A20 kernel_main

# 2. Vérifier utilisation de registres 64-bit
objdump -d iso/boot/kernel | grep -E "mov.*%rdi|mov.*%rsi"

# 3. Test boot
timeout 3 qemu-system-x86_64 -M q35 -m 128 -cdrom kernel.iso \
    -serial file:serial-test.log
cat serial-test.log
# Doit afficher exactement 51 bytes:
# BareFlow Zig Kernel\n
# Serial I/O Test\n
# Boot complete!\n
```

---

## Fichiers Concernés

- `build.zig` - Documentation du workaround
- `build_simple.sh` - Script de compilation manuelle
- `src/main_simple.zig` - Code kernel

---

## Référence

- Session 47, commit 3d87c24
- `SESSION_47_STRING_POINTER_FIX.md`
