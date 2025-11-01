# Issues Resolved - Base de Données des Problèmes

**Objectif**: Documenter tous les problèmes rencontrés et leurs solutions pour éviter de répéter les mêmes erreurs.

**Utilisation**: Consulter ce document AVANT d'implémenter de nouvelles fonctionnalités ou lors du debugging.

---

## Table des Matières

1. [Boot Issues](#boot-issues)
2. [Compilation Issues](#compilation-issues)
3. [Runtime Issues](#runtime-issues)
4. [Linker Issues](#linker-issues)
5. [QEMU/Testing Issues](#qemutesting-issues)

---

## Boot Issues

### 🔴 CRITIQUE #1: Kernel ne boot pas - CPU reset immédiat

**Session**: 46, 47
**Symptômes**:
- QEMU affiche "Booting from CD-ROM" puis reset immédiat
- Aucune sortie série
- CPU triple fault ou exception non gérée

**Causes Possibles**:

#### 1.1 Multiboot2 Header Invalide
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

#### 1.2 BSS Non-Zeroed (Pages Tables Corrompues)
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

#### 1.3 Mismatch 32-bit Boot / 64-bit Kernel
**Problème**: boot.S (32-bit) essaie de charger kernel 64-bit
**Symptôme**: Kernel ne démarre jamais après "Booting from CD-ROM"
**Solution**: Utiliser boot64.S (native 64-bit) avec GRUB Multiboot2

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

#### 1.4 Stack Overflow / Red Zone Corruption
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

## Compilation Issues

### 🔴 CRITIQUE #2: Zig génère des adresses 32-bit au lieu de 64-bit

**Session**: 47 Part 2
**Symptômes**:
- Infinite 'E' characters sur sortie série
- Garbage data au lieu des strings attendus
- Kernel semble "boucler" sans raison

**Diagnostic**:
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

**Cause**: Flag `-mcmodel=kernel` seulement sur boot64.S, pas sur code Zig

**Solution**:
```bash
# Compilation manuelle avec code model kernel
zig build-obj -target x86_64-freestanding -mcmodel=kernel \
    -O ReleaseSafe src/main_simple.zig -femit-bin=main_simple.o
```

**Bonus**: Avec `-mcmodel=kernel`, Zig inline complètement `serial_print()`:
```assembly
# Plus de pointeurs du tout! Sortie directe des caractères
mov $0x42,%al   # 'B'
out %al,(%dx)
mov $0x61,%al   # 'a'
out %al,(%dx)
```

**Référence**: Session 47, commit 3d87c24, `SESSION_47_STRING_POINTER_FIX.md`

### 🟡 #3: Property 'code_model' n'existe pas dans Zig 0.13

**Session**: 47
**Symptôme**: Build hang ou erreur de propriété inconnue
**Solution**: Utiliser compilation manuelle, pas build.zig

```zig
// ❌ NE FONCTIONNE PAS (Zig 0.13)
kernel.root_module.code_model = .kernel;

// ✅ WORKAROUND - Documenter dans build.zig
// NOTE (Session 47): kernel.root_module.code_model property does not exist in Zig 0.13
// Manual compilation required:
//   zig build-obj -mcmodel=kernel src/main_simple.zig
```

**Référence**: build.zig lignes 38-43

### 🟡 #4: Cannot use addCSourceFile for .zig files

**Session**: 47
**Symptôme**: Flags pas appliqués aux fichiers Zig
**Solution**: `addCSourceFile()` est pour C uniquement, pas Zig

```zig
// ❌ NE FONCTIONNE PAS
kernel.root_module.addCSourceFile(.{
    .file = b.path("src/main_simple.zig"),  // Zig file!
    .flags = &.{"-mcmodel=kernel"},
});

// ✅ CORRECT - Compilation manuelle
// Voir build_simple.sh
```

---

## Runtime Issues

### 🔴 CRITIQUE #5: Serial I/O - Infinite 'E' characters

**Session**: 47
**Symptômes**:
- Single character works: `serial_write('A')` ✅
- Single string works: `serial_print("ABC\n")` ✅
- Multiple strings fail: Infinite 'E' or garbage ❌

**Cause**: Voir [Issue #2](#🔴-critique-2-zig-génère-des-adresses-32-bit-au-lieu-de-64-bit) - Adressage 32-bit

**User Insight**: *"il ne pointe pas vers la bonne adresse et donc ne rencontre jamais le \0"*

**Solution**: Compiler avec `-mcmodel=kernel`

**Validation**:
```bash
timeout 3 qemu-system-x86_64 -M q35 -m 128 -cdrom bareflow-simple.iso \
    -serial file:serial-test.log
cat serial-test.log
# Doit afficher exactement 51 bytes:
# BareFlow Zig Kernel\n
# Serial I/O Test\n
# Boot complete!\n
```

### 🟡 #6: Serial Polling Too Strict

**Session**: 47
**Symptôme**: Serial output parfois manquant
**Problème**: Polling THRE + TEMT (mask 0x60) trop strict

```zig
// ❌ Trop strict
while ((inb(COM1 + 5) & 0x60) != 0x60) {}

// ✅ Standard UART - THRE seulement
while ((inb(COM1 + 5) & 0x20) == 0) {}
```

**Note**: Ce n'était PAS la cause du bug de pointeurs, mais c'est la bonne pratique.

### 🟡 #7: VGA Buffer Addressing

**Session**: 47
**Symptôme**: VGA output ne s'affiche pas
**Solution**: VGA buffer à 0xB8000 (identity mapped ou higher-half)

```zig
const VGA_BUFFER = 0xB8000;  // Physical address
// OU
const VGA_BUFFER = 0xFFFFFFFF800B8000;  // Higher-half
```

---

## Linker Issues

### 🟡 #8: Section Alignment Syntax Error

**Session**: 46
**Symptôme**: `ld.lld-18: error: malformed number: ALIGN`
**Solution**: Utiliser syntaxe correcte pour ALIGN()

```ld
/* ❌ CASSÉ */
.bss ALIGN(4096) : {

/* ✅ CORRECT */
.bss : ALIGN(4096) {
```

**Référence**: src/linker.ld

### 🟡 #9: BSS Boundaries Not Defined

**Session**: 46
**Symptôme**: BSS zeroing échoue silencieusement
**Solution**: Définir __bss_start et __bss_end dans linker script

```ld
.bss : ALIGN(4096) {
    __bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    __bss_end = .;
}
```

Usage dans boot64.S:
```assembly
mov $__bss_start, %rdi
mov $__bss_end, %rcx
sub %rdi, %rcx
xor %rax, %rax
rep stosb
```

### 🔴 CRITIQUE #10: PIE (Position Independent Executable) Enabled

**Session**: 46, 47
**Symptôme**: Relocations dans le binaire, GRUB rejette
**Solution**: Désactiver PIE explicitement

```zig
// build.zig
kernel.pie = false;
```

```bash
# Compilation manuelle
gcc -fno-pie ...
ld.lld-18 -no-pie ...
```

**Validation**:
```bash
readelf -h iso/boot/kernel | grep Type
# Doit afficher: Type: EXEC (Executable file)
# PAS: Type: DYN (Shared object file)
```

---

## QEMU/Testing Issues

### 🟡 #11: QEMU ne démarre pas avec -kernel

**Session**: 46, 47
**Symptôme**: `qemu-system-x86_64 -kernel kernel` → erreur ou hang
**Solution**: Utiliser ISO + GRUB, pas -kernel direct

```bash
# ❌ NE FONCTIONNE PAS (Multiboot2)
qemu-system-x86_64 -kernel iso/boot/kernel

# ✅ FONCTIONNE - ISO bootable
grub-mkrescue -o bareflow.iso iso/
qemu-system-x86_64 -M q35 -m 128 -cdrom bareflow.iso -serial stdio
```

**Pourquoi**: QEMU `-kernel` supporte Multiboot1, pas Multiboot2

### 🟡 #12: Serial Output Non Visible

**Session**: 46, 47
**Symptôme**: Kernel s'exécute mais pas de sortie
**Solutions**:

```bash
# Option 1: Redirection vers fichier
qemu-system-x86_64 -serial file:serial.log -cdrom bareflow.iso

# Option 2: Sortie stdio (terminal)
qemu-system-x86_64 -serial stdio -cdrom bareflow.iso

# Option 3: Debugcon (early boot)
qemu-system-x86_64 -debugcon file:debug.log -cdrom bareflow.iso
```

**Usage debugcon dans boot64.S**:
```assembly
# Port 0xE9 - QEMU debugcon
mov $0xE9, %dx
mov $'B', %al    # Signal 'B' for "BSS zeroed"
out %al, (%dx)
```

### 🟡 #13: ISO Checksum Different

**Session**: 47
**Symptôme**: ISO rebuilds avec checksums différents
**Cause**: Timestamps dans ISO filesystem
**Solution**: Vérifier le KERNEL, pas l'ISO

```bash
# ❌ Ne compare pas les ISO
sha256sum bareflow.iso bareflow-new.iso

# ✅ Compare les kernels
sudo mount bareflow.iso /mnt -o loop
sha256sum /mnt/boot/kernel
sudo umount /mnt
```

---

## Quick Reference - Commandes Essentielles

### Validation Pre-Boot
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
```

### Debugging Serial I/O
```bash
# 1. Test avec serial output
timeout 3 qemu-system-x86_64 -M q35 -m 128 -cdrom kernel.iso \
    -serial file:serial.log
cat serial.log

# 2. Vérifier byte count
wc -c serial.log
xxd serial.log | head -10

# 3. Désassembler kernel_main
objdump -d iso/boot/kernel | grep -A20 kernel_main
```

### Debugging Boot Sequence
```bash
# 1. Debugcon early boot
timeout 3 qemu-system-x86_64 -cdrom kernel.iso \
    -debugcon file:debug.log -no-reboot
cat debug.log

# 2. CPU debug logs
qemu-system-x86_64 -cdrom kernel.iso \
    -d cpu_reset,guest_errors -no-reboot

# 3. Interrupt debug
qemu-system-x86_64 -cdrom kernel.iso \
    -d int -no-reboot
```

---

## Workflow de Resolution

### Étape 1: Identifier le Symptôme
- [ ] Kernel ne boot pas → [Boot Issues](#boot-issues)
- [ ] Kernel boot mais crash → [Runtime Issues](#runtime-issues)
- [ ] Compilation échoue → [Compilation Issues](#compilation-issues)
- [ ] Linking échoue → [Linker Issues](#linker-issues)

### Étape 2: Validation Systématique
1. **Multiboot2**: `grub-file --is-x86-multiboot2`
2. **ELF Type**: `readelf -h | grep Type` → EXEC
3. **Entry Point**: `readelf -h | grep Entry`
4. **BSS Zeroing**: Vérifier code dans boot64.S
5. **Code Model**: `objdump -d | grep kernel_main` → Vérifier RDI (64-bit)

### Étape 3: Debugging Progressif
1. **Boot**: Debugcon → CPU logs → Serial output
2. **Runtime**: Serial I/O → VGA buffer → GDB
3. **Compilation**: Disassembly → Section dump → Symbol table

### Étape 4: Documentation
- [ ] Ajouter problème à ce document
- [ ] Créer session document si investigation longue
- [ ] Commit avec message détaillé
- [ ] Mettre à jour CLAUDE.md si pattern récurrent

---

## Patterns Récurrents

### 🔁 Pattern #1: "Kernel boot mais pas de sortie"
**Checklist**:
1. Serial init appelé? (`serial_init()` dans kernel_main)
2. QEMU avec `-serial stdio` ou `-serial file:log`?
3. COM1 port correct? (0x3F8)
4. Polling correct? (THRE bit 5, mask 0x20)

### 🔁 Pattern #2: "Compilation OK mais linking échoue"
**Checklist**:
1. Linker script présent? (`-T src/linker.ld`)
2. Sections définies? (.text, .rodata, .data, .bss)
3. Entry point déclaré? (`ENTRY(kernel_entry)`)
4. BSS boundaries? (`__bss_start`, `__bss_end`)

### 🔁 Pattern #3: "GRUB ne reconnaît pas le kernel"
**Checklist**:
1. Multiboot2 header? (magic 0xE85250D6)
2. Architecture i386? (pas x86_64 pour Multiboot2!)
3. Checksum correct?
4. Section .multiboot en début de fichier?

### 🔁 Pattern #4: "Pointeurs/adresses invalides"
**Checklist**:
1. Code model kernel? (`-mcmodel=kernel`)
2. Red zone désactivée? (`-mno-red-zone`)
3. PIE désactivé? (`-fno-pie`, `kernel.pie = false`)
4. Registres 64-bit utilisés? (RDI, RSI, RDX - pas EDI, ESI, EDX)

---

## Lessons Learned - Best Practices

### ✅ DO: Toujours Faire
1. **Compile avec `-mcmodel=kernel`** pour code 64-bit kernel
2. **Désactiver PIE** (`kernel.pie = false`)
3. **Désactiver red zone** (`-mno-red-zone`)
4. **Zeroer BSS** avant utilisation (page tables, stack, etc.)
5. **Valider Multiboot2** avec `grub-file`
6. **Tester dans QEMU** avant hardware
7. **Vérifier disassembly** pour bugs de compilation
8. **Documenter IMMÉDIATEMENT** après résolution

### ❌ DON'T: Ne Jamais Faire
1. **Assumer que userspace = bare-metal** → Toujours tester QEMU
2. **Utiliser `-kernel` avec Multiboot2** → Utiliser ISO + GRUB
3. **Oublier BSS zeroing** → Page tables corrompues
4. **Mixer 32-bit/64-bit** → Utiliser architecture consistante
5. **Ignorer les warnings** → Souvent signes de vrais problèmes
6. **Commit sans test** → Un bug peut bloquer des heures

### 🔍 Debug: Méthode Systématique
1. **Reproduce**: Isoler le problème (minimal test case)
2. **Validate**: Vérifier les hypothèses (disassembly, hexdump)
3. **Search**: Consulter ce document + git history
4. **Test**: Valider la fix dans QEMU
5. **Document**: Ajouter à ce fichier si nouveau problème

---

## Historique des Sessions

| Session | Problème Principal | Solution | Référence |
|---------|-------------------|----------|-----------|
| 46 | Boot failures multiples | BSS zeroing, debugcon, PIC masking | `SESSION_46_FINAL.md` |
| 47-1 | 32-bit/64-bit mismatch | boot64.S native 64-bit | `SESSION_47_BREAKTHROUGH.md` |
| 47-2 | String pointer bug (32-bit addressing) | `-mcmodel=kernel` compilation | `SESSION_47_STRING_POINTER_FIX.md` |

---

## Contribuer à ce Document

### Quand Ajouter un Problème
- Problème pris > 30 minutes à résoudre
- Problème susceptible de se répéter
- Solution non-évidente ou contre-intuitive
- Problème lié à configuration/tooling

### Format d'Entrée
```markdown
### 🔴 CRITIQUE #N: Titre Court du Problème

**Session**: XX
**Symptômes**:
- Comportement observable 1
- Comportement observable 2

**Cause**: Explication technique

**Solution**: Code ou commande

**Validation**: Comment vérifier que c'est fixé

**Référence**: Liens vers docs/commits
```

### Niveaux de Sévérité
- 🔴 **CRITIQUE**: Bloque le boot ou cause data loss
- 🟡 **MOYEN**: Impacte fonctionnalité mais workaround existe
- 🟢 **FAIBLE**: Inconvénient mineur

---

**Dernière Mise à Jour**: 2025-11-01 (Session 47)
**Maintenu par**: Claude Code + @nickywan
**Version**: 1.0
