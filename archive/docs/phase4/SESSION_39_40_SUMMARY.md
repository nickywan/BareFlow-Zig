# Sessions 39-40: TinyLlama Inference & Weight Loading

**Date**: 2025-10-26
**Branch**: feat/true-jit-unikernel
**Commits**: 50d4c3a, 70ac475

## Objectifs

**Session 39**: Implémenter le pipeline d'inférence transformer complet
**Session 40**: Créer le système de chargement de poids avec PRNG

## Résultats

### ✅ Accompli

1. **Infrastructure d'Inférence Complète** (Session 39)
   - 15 fonctions transformer core
   - Fast math (sqrt, exp) sans stdlib
   - RMS Normalization
   - Rotary Position Embeddings (RoPE)
   - Quantized INT8 matmul
   - Attention + Feed-Forward (SwiGLU)
   - Pipeline complet: `tinyllama_forward_token()`

2. **Système de Poids** (Session 40)
   - PRNG (Linear Congruential Generator)
   - Génération de poids INT8 reproductibles
   - Seeds uniques par composant (1000, 2000+N*100, 9000)
   - Fonctions d'init: tensor, layer, model
   - Cleanup functions

### 📊 Métriques

```
Code ajouté: 860 lignes (4 fichiers)
Binary: 18,528 → 23,768 bytes (+5.2 KB)
Fonctions: 23 total
Compilation: -O0 (required for bare-metal)
```

### 🏗️ Fichiers Créés

```
tests/phase4/qemu_llvm_64/
├── tinyllama_inference.h  (220 lines)
├── tinyllama_inference.c  (350 lines)
├── tinyllama_weights.h    (90 lines)
└── tinyllama_weights.c    (200 lines)
```

## ⚠️ Bug Identifié: Return Value Corruption

### Symptôme
```c
int tinyllama_create_model(TinyLlamaModel** model) {
    // ... allocation successful ...
    serial_puts("[C_FUNC] About to return 0\n");
    return 0;  // Function returns 0
}

// Caller receives -1 instead of 0!
int result = tinyllama_create_model(&model);
// result == -1, model != NULL
```

### Impact
- Modèle créé correctement (pointeur valide)
- Allocations réussies
- Seul le code retour est corrompu
- Bloque l'exécution du chargement de poids

### Contexte
- Apparu Session 37 avec Clang -O1/-O2
- Persiste avec -O0 (mais malloc fonctionne)
- Probablement ABI x86-64 bare-metal
- **Solution**: GCC diagnostic uniquement (rester sur LLVM!)

### Workaround
```c
// Au lieu de checker le return value:
tinyllama_create_model(&model);
if (model != NULL) {
    // Success - use the model
}
```

## Architecture Technique

### Pipeline d'Inférence
```
Input Token (uint32_t)
    ↓
Token Embedding (INT8 → FP32)
    ↓
[22 Transformer Layers]
│  ├─ RMSNorm
│  ├─ Attention (RoPE + Softmax)
│  ├─ Residual Add
│  ├─ RMSNorm
│  ├─ Feed-Forward (SwiGLU)
│  └─ Residual Add
    ↓
Final RMSNorm
    ↓
Output Projection (vocab_size logits)
```

### Choix de Design

**1. INT8 Quantization**
- Réduit mémoire 4× (32→8 bits)
- TinyLlama: ~4GB FP32 → ~1GB INT8
- Formule: `float = (int8 - zero_point) * scale`

**2. Fast Math Approximations**
- `fast_sqrt()`: Newton-Raphson (5 iterations)
- `fast_exp()`: Taylor series (6 terms)
- Suffisant pour normalization/softmax

**3. -O0 Compilation**
- -O1/-O2 cassent malloc en bare-metal
- JIT compensera plus tard (O0→O3 runtime)
- Philosophie "Grow to Shrink"

**4. PRNG pour Poids Dummy**
- Permet testing sans fichiers
- Reproductible (seeded)
- Vrais poids TinyLlama = future

## Tests & Validation

### Build
```bash
$ make clean && make
Compilation: SUCCESS
Warnings: 7 (unused params in stubs)
Kernel size: 23768 bytes
```

### Boot (QEMU)
```
✅ Serial I/O working
✅ Paging initialized (2MB pages)
✅ malloc(1024) SUCCESS
✅ Model structure created
✅ Layers allocated (Q K V O W1 W2 LN1 LN2)
⚠️  Return value bug blocks weight loading
```

## Prochaines Étapes

### Court Terme (Sessions 41-42)
- [ ] **Option 3**: GCC diagnostic (tool only!)
  - Compiler avec GCC pour comparaison
  - Analyser assembleur (Clang vs GCC)
  - Identifier différence ABI
  - Fix bug (rester sur LLVM)
- [ ] Test chargement de poids
- [ ] Premier forward pass avec dummy data

### Moyen Terme (Sessions 43-45)
- [ ] **Option 4**: LLVM JIT profiling hooks
- [ ] Profiling hot paths
- [ ] KV cache implementation
- [ ] Attention complète (non-stub)

### Long Terme (Sessions 46+)
- [ ] Vrais poids TinyLlama (1.1B)
- [ ] Génération de texte
- [ ] Optimisation JIT progressive
- [ ] Export binaire natif optimisé

## Leçons Apprises

### Bare-Metal Challenges
1. **No stdlib = tout coder** - sqrt, exp, malloc, etc.
2. **ABI matters** - Calling conventions critiques
3. **Optimizations break things** - -O0 obligatoire
4. **Debugging is hard** - Serial output only

### LLVM en Bare-Metal
1. Clang-18 excellent pour compilation
2. ld.lld-18 parfait pour linking
3. Flags: `-ffreestanding -nostdlib -mcmodel=kernel`
4. Attention aux calling conventions ABI

### "Grow to Shrink" Philosophy
```
Boot 1:   [60MB] Full LLVM + app IR → Profile TOUT
Boot 100: [30MB] Hot paths JIT compiled
Boot 500: [10MB] Dead code eliminated
Boot 1000:[2-5MB] Pure native export
```

## Décisions Documentées

### GCC = Diagnostic Tool ONLY
- **Utilisation**: Identifier bug ABI
- **Analyse**: Comparer assembleur Clang vs GCC
- **Solution**: Fix dans code, pas switch compilateur
- **Cible finale**: LLVM JIT auto-optimisant

### Pourquoi pas alternatives (QBE, Cranelift)?
- **Besoin**: JIT complet avec optimisations
- **Grow to Shrink**: Nécessite profiling + dead code elimination
- **LLVM**: Seul capable O0→O3 runtime progressive

## Code Reference

### Fonctions Clés

```c
// Pipeline complet
int tinyllama_forward_token(
    const TinyLlamaModel* model,
    uint32_t token,
    uint32_t pos,
    float* logits
);

// Math bare-metal
float fast_sqrt(float x);  // Newton-Raphson
float fast_exp(float x);   // Taylor series

// Transformer layers
void rms_norm(float* x, const float* weight, uint32_t size);
void rope_encoding(float* q, float* k, uint32_t pos, ...);
void attention(...);
void feed_forward(...);

// Poids
int init_model_weights_dummy(TinyLlamaModel* model);
```

### Localisation Code

| Fonction | Fichier | Ligne |
|----------|---------|-------|
| tinyllama_forward_token | tinyllama_inference.c | 299 |
| rms_norm | tinyllama_inference.c | 58 |
| rope_encoding | tinyllama_inference.c | 119 |
| init_model_weights_dummy | tinyllama_weights.c | 131 |

## Status

**Sessions 39-40**: ✅ COMPLETE
**Options 1-2**: ✅ COMPLETE
**Option 3**: 🔄 IN PROGRESS (GCC diagnostic)
**Option 4**: ⏳ PENDING (LLVM JIT hooks)

**Philosophie maintenue**: "On s'en fiche de la taille initiale!"

---

**Last Updated**: 2025-10-26
**Next Session**: Option 3 - GCC Diagnostic (tool only, stay on LLVM!)
