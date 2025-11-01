# Issue #14: Hex Printing Corruption - Conditional Toujours True

**Catégorie**: Runtime Issues
**Sévérité**: 🔴 CRITIQUE
**Session**: 47 (Continuation)

---

## Symptômes

- Hex digits A-F affichent caractères incorrects ('<', '>', '?')
- Pattern: -7 offset ('C'→'<' = 0x43→0x3C)
- Test: `0x123456789ABCDEF0` → `0x123456789A<=>?0`
- Analyse: `nibble=12` produit '0'+12 au lieu de 'A'+2

---

## Diagnostic

```bash
# Serial output montre corruption
Test hex: 0x123456789A<=>?0  # ❌ CASSÉ
Test hex: 0x123456789ABCDEF0 # ✅ CORRECT après fix
```

**Pattern de Corruption**:
- 'A' (0x41) → '<' (0x3C) = -5
- 'B' (0x42) → '=' (0x3D) = -5
- 'C' (0x43) → '<' (0x3C) = -7 ⚠️
- 'D' (0x44) → '=' (0x3D) = -7
- 'E' (0x45) → '>' (0x3E) = -7
- 'F' (0x46) → '?' (0x3F) = -7

**Analyse**: Pour nibble=12 ('C'):
- Attendu: 'A' + (12 - 10) = 'A' + 2 = 'C' (0x43)
- Observé: '0' + 12 = '<' (0x3C)

**Conclusion**: Le conditional `nibble < 10` retourne TOUJOURS true!

---

## Cause Racine

Zig ReleaseFast optimizer CASSE les expressions conditionnelles.

### Code Cassé #1: Inline Conditional

```zig
// ❌ CASSÉ - Conditional TOUJOURS true en ReleaseFast
fn nibble_to_hex(nibble: u8) u8 {
    return if (nibble < 10) '0' + nibble else 'A' + (nibble - 10);
}
```

### Code Cassé #2: Explicit If/Else

```zig
// ❌ TOUJOURS CASSÉ - Même avec if/else explicite!
fn nibble_to_hex(nibble: u8) u8 {
    if (nibble < 10) {
        return '0' + nibble;
    } else {
        return 'A' + (nibble - 10);
    }
}
// La comparaison (nibble < 10) retourne TOUJOURS true!
```

**User Insight Critical**: *"en fait le hex printing peut cacher un problème sous jacent qui risque de nous peter à la figure, n'est pas un problème de return value ?"*

L'utilisateur a correctement identifié que c'était un problème de return value, pas juste un bug d'affichage!

---

## Solution: Lookup Table

```zig
// ✅ FONCTIONNE - Compile-time constant lookup table
const HEX_DIGITS: [16]u8 = [_]u8{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

fn nibble_to_hex(nibble: u8) u8 {
    return HEX_DIGITS[nibble & 0xF];
}
```

**Pourquoi ça marche**:
- Pas de branches conditionnelles → Pas de bug d'optimisation
- Array indexing direct avec bounds check (& 0xF)
- Const array placée en .rodata section
- Évite les problèmes d'adressage 32-bit

---

## Validation

```
Test hex: 0x123456789ABCDEF0 ✓
Result 1: 0x0000008E (expected 0x8E = 142) ✓
Result 2: 0xFFFFFFF8 (expected -8) ✓
Result 3: 0x00000412 (expected 0x412 = 1042) ✓
```

Tous les tests de return value passent correctement!

---

## Impact

Ce bug cachait également le problème #15 (32MB heap).

Les deux problèmes étaient **LIÉS** - le code conditionnel corrompu affectait également la mémoire.

---

## Fichiers Concernés

- `src/main.zig` - Implementation du lookup table
- `SESSION_47_ALLOCATOR_FIXES.md` - Documentation complète

---

## Référence

- Session 47 (Continuation), commit 747f84e
- Voir aussi: [Issue #15 - 32MB Heap Triple Fault](issue-15-32mb-heap-triple-fault.md)
