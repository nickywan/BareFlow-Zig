# Phase 5 Planning: TinyLlama Model Integration

**Date**: 2025-10-26
**Status**: 📝 READY TO START
**Prerequisites**: Phase 4 complete ✅
**Duration**: Sessions 31-39 (~3-4 weeks)

---

## Overview

Phase 5 focuses on integrating the TinyLlama language model with the bare-metal runtime and enabling JIT optimization of inference code. This phase implements the core "Grow to Shrink" strategy: start with full LLVM + model, profile execution, JIT-optimize hot paths, and converge to a minimal optimized binary.

**Key Goal**: Self-optimizing AI appliance that boots with everything embedded and converges to hardware-optimal performance.

---

## Critical Prerequisite: Paging (Session 31)

**Status**: MUST be implemented first
**Reason**: malloc() triple fault requires paging for 64-bit memory writes

### Why Paging is Required

From malloc investigation (Session 29):
- Crash point: Writing to `free_list` global variable
- Root cause: 64-bit long mode requires paging for full memory access
- Current state: No page tables set up
- Impact: Cannot allocate memory for LLVM or model weights

### Paging Implementation Plan

**Architecture**: x86-64 4-level paging
```
Virtual Address (64-bit):
[63:48] Sign extension (16 bits)
[47:39] PML4 index (9 bits) - 512 entries
[38:30] PDPT index (9 bits) - 512 entries
[29:21] PD index (9 bits)   - 512 entries
[20:12] PT index (9 bits)   - 512 entries
[11:0]  Offset (12 bits)    - 4 KB pages
```

**Memory Map**:
```
0x0000000000000000 - 0x0000000000100000  Identity mapped (1 MB - BIOS/boot)
0x0000000000100000 - 0x0000000000400000  Kernel (.text, .data, .bss) - Identity mapped
0x0000000000400000 - 0x0000000010000000  Heap (up to 256 MB) - Writable
0xFFFF800000000000 - ...                 Higher-half (optional, future)
```

**Page Table Structure** (to create in boot.S):
```asm
.section .bss
.align 4096
pml4_table:
    .skip 4096      # PML4 (512 entries × 8 bytes)
pdpt_table:
    .skip 4096      # PDPT (512 entries × 8 bytes)
pd_tables:
    .skip 8192      # 2× PD (2× 512 entries) - covers 1 GB
pt_tables:
    .skip 8192      # 2× PT (2× 512 entries) - covers 4 MB kernel
```

**Implementation Steps** (Session 31):
1. Create page table structures in .bss
2. Initialize PML4 → PDPT → PD → PT chain
3. Identity map 0-4 MB (kernel region)
4. Set page flags (Present, Writable, User accessible)
5. Load CR3 with PML4 address
6. Enable paging (CR0.PG bit already set by Multiboot2?)
7. Test malloc with paging enabled

**Expected Result**: malloc() works without triple fault

---

## Session-by-Session Plan

### Session 31: Paging Implementation ✅ CRITICAL

**Goals**:
- [x] Implement 4-level page tables
- [x] Identity map kernel (0-4 MB)
- [x] Enable paging in boot.S
- [x] Test malloc_llvm.c
- [x] Validate in QEMU

**Tasks**:
1. Create page table data structures (.bss section)
2. Write init_paging() in boot.S:
   - Zero page tables
   - Setup PML4 entry → PDPT
   - Setup PDPT entry → PD
   - Setup PD entries → PT (identity map 4 MB)
   - Set Present + Writable flags
3. Load CR3 register with PML4 address
4. Verify paging enabled (Multiboot2 should enable)
5. Test malloc(1024), malloc(1 MB), malloc(10 MB)
6. Test free() and coalescing

**Deliverables**:
- boot.S with init_paging()
- Working malloc in QEMU
- Paging test results documented

**Risks**:
- Page fault if mapping incorrect
- Triple fault if PML4/CR3 wrong
- Performance impact (TLB misses)

**Mitigation**:
- Page fault handler (IDT entry 14) for debugging
- Granular logging in init_paging()
- QEMU monitor commands to inspect page tables

---

### Session 32: Model Weight Format Design

**Goals**:
- [ ] Design binary weight format (.bin)
- [ ] Create weight loader in bare-metal
- [ ] Test with sample weights (small matrix)
- [ ] Document format specification

**Weight Format Design**:

```c
// weights.bin format (little-endian)

struct WeightHeader {
    uint32_t magic;           // 0x574D4C4C ("WLLM")
    uint32_t version;         // 1
    uint32_t model_type;      // 1 = TinyLlama-1.1B
    uint32_t num_layers;      // 22 (TinyLlama)
    uint32_t hidden_size;     // 2048
    uint32_t intermediate_size; // 5632
    uint32_t vocab_size;      // 32000
    uint32_t num_heads;       // 32
    uint64_t total_size;      // Total weight data size (bytes)
};

struct LayerWeights {
    // Attention
    float* q_weight;          // [hidden_size × hidden_size]
    float* k_weight;          // [hidden_size × hidden_size]
    float* v_weight;          // [hidden_size × hidden_size]
    float* o_weight;          // [hidden_size × hidden_size]

    // Feed-forward
    float* gate_weight;       // [hidden_size × intermediate_size]
    float* up_weight;         // [hidden_size × intermediate_size]
    float* down_weight;       // [intermediate_size × hidden_size]

    // Norms
    float* attn_norm;         // [hidden_size]
    float* ffn_norm;          // [hidden_size]
};

struct TinyLlamaWeights {
    WeightHeader header;
    float* embedding;         // [vocab_size × hidden_size]
    LayerWeights layers[22];
    float* final_norm;        // [hidden_size]
    float* lm_head;           // [hidden_size × vocab_size]
};
```

**Loader Implementation**:
```c
// kernel_lib/model/weight_loader.h
TinyLlamaWeights* load_weights(const char* filename);
void free_weights(TinyLlamaWeights* weights);
```

**Storage Options**:
1. **RAM disk** (simple, volatile) - Load from address
2. **FAT16** (persistent) - Read from filesystem
3. **Embedded** (compile-time) - Include in binary

**Decision**: Start with RAM disk (simplest), add FAT16 in Session 33.

**Deliverables**:
- Weight format specification
- weight_loader.c implementation
- Test with dummy weights (100 KB)

---

### Session 33: Model Loading & Initialization

**Goals**:
- [ ] Load TinyLlama weights (~60 MB)
- [ ] Initialize model structure
- [ ] Create inference skeleton
- [ ] Test forward pass (single token)

**Implementation**:

```c
// tinyllama/model.h

typedef struct {
    TinyLlamaWeights* weights;
    float* kv_cache;           // Key-value cache [layers × 2 × seq_len × hidden]
    float* logits;             // Output logits [vocab_size]
    int seq_len;
    int max_seq_len;
} TinyLlamaModel;

TinyLlamaModel* model_init(const char* weights_path, int max_seq_len);
void model_forward(TinyLlamaModel* model, int* tokens, int n_tokens);
void model_free(TinyLlamaModel* model);
```

**Forward Pass Skeleton**:
```c
void model_forward(TinyLlamaModel* model, int* tokens, int n_tokens) {
    // 1. Embedding lookup
    float* hidden = embed_tokens(model->weights, tokens, n_tokens);

    // 2. Layer loop
    for (int layer = 0; layer < 22; layer++) {
        // Attention
        hidden = attention_layer(model, layer, hidden);

        // Feed-forward
        hidden = ffn_layer(model, layer, hidden);
    }

    // 3. Final norm + LM head
    model->logits = lm_head(model->weights, hidden);
}
```

**Memory Requirements**:
```
Weights:          ~60 MB (fp32)
KV cache:         ~50 MB (seq_len=2048, 22 layers)
Activations:      ~20 MB (intermediate buffers)
Total:            ~130 MB
```

**Heap Configuration**:
```c
#define HEAP_SIZE (256 * 1024 * 1024)  // 256 MB heap
```

**Deliverables**:
- model.c implementation
- Forward pass working (slow, unoptimized)
- Inference test: "Hello" → next token

**Expected Performance**: ~10-30 seconds per token (unoptimized)

---

### Session 34: Matrix Multiply Profiling

**Goals**:
- [ ] Profile matrix multiply calls
- [ ] Identify hot matrix shapes
- [ ] Measure baseline performance
- [ ] Design JIT specialization strategy

**Profiling Infrastructure**:

```c
// kernel_lib/jit/matrix_profile.h

typedef struct {
    int m, n, k;              // Matrix dimensions (M×K) @ (K×N)
    uint64_t call_count;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
} MatMulProfile;

void matmul_profile_begin(int m, int n, int k);
void matmul_profile_end(void);
MatMulProfile* matmul_get_hottest(int* count);
```

**Expected Hot Shapes** (TinyLlama):
```
Q/K/V projections:    [seq_len × 2048] @ [2048 × 2048]
O projection:         [seq_len × 2048] @ [2048 × 2048]
Gate/Up:              [seq_len × 2048] @ [2048 × 5632]
Down:                 [seq_len × 5632] @ [5632 × 2048]
```

**Baseline Implementation** (generic, no optimization):
```c
void matmul_f32(float* C, const float* A, const float* B, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i*K + k] * B[k*N + j];
            }
            C[i*N + j] = sum;
        }
    }
}
```

**Profiling Output**:
```
Matrix Multiply Profiling (10 tokens):

Shape [1×2048]@[2048×2048]:   220 calls,  45.2ms avg  ← HOT! (Q/K/V/O)
Shape [1×2048]@[2048×5632]:    66 calls,  80.1ms avg  ← HOT! (Gate/Up)
Shape [1×5632]@[5632×2048]:    66 calls,  92.3ms avg  ← HOTTEST! (Down)

Total matmul time: 98.7% of inference time
```

**Deliverables**:
- Profiling infrastructure
- Baseline performance measurements
- Hot shape identification
- JIT optimization targets defined

---

### Session 35: CPU Feature Detection & Vectorization

**Goals**:
- [ ] Detect CPU features (CPUID)
- [ ] Implement SSE2 matrix multiply
- [ ] Implement AVX2 matrix multiply (if available)
- [ ] Benchmark vectorized versions

**CPU Feature Detection**:

```c
// kernel_lib/cpu/features.h

typedef struct {
    bool sse2;
    bool sse3;
    bool ssse3;
    bool sse4_1;
    bool sse4_2;
    bool avx;
    bool avx2;
    bool fma;
    bool avx512f;
} CpuFeatures;

CpuFeatures cpu_detect_features(void);
```

**SSE2 Matrix Multiply** (4 floats at a time):
```c
void matmul_f32_sse2(float* C, const float* A, const float* B, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j += 4) {  // Process 4 columns at once
            __m128 sum = _mm_setzero_ps();
            for (int k = 0; k < K; k++) {
                __m128 a = _mm_set1_ps(A[i*K + k]);
                __m128 b = _mm_loadu_ps(&B[k*N + j]);
                sum = _mm_add_ps(sum, _mm_mul_ps(a, b));
            }
            _mm_storeu_ps(&C[i*N + j], sum);
        }
    }
}
```

**AVX2 Matrix Multiply** (8 floats at a time):
```c
void matmul_f32_avx2(float* C, const float* A, const float* B, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j += 8) {  // Process 8 columns at once
            __m256 sum = _mm256_setzero_ps();
            for (int k = 0; k < K; k++) {
                __m256 a = _mm256_set1_ps(A[i*K + k]);
                __m256 b = _mm256_loadu_ps(&B[k*N + j]);
                sum = _mm256_fmadd_ps(a, b, sum);  // FMA: sum += a * b
            }
            _mm256_storeu_ps(&C[i*N + j], sum);
        }
    }
}
```

**Expected Speedup**:
```
Baseline (scalar):     100 ms/token
SSE2 (4-wide):         ~40 ms/token   (2.5× speedup)
AVX2 (8-wide + FMA):   ~15 ms/token   (6.5× speedup)
```

**Deliverables**:
- CPU feature detection
- SSE2 implementation
- AVX2 implementation
- Benchmark results

---

### Session 36: JIT Matrix Specialization

**Goals**:
- [ ] Generate specialized matrix multiply with LLVM
- [ ] Hardcode matrix dimensions (unroll loops)
- [ ] Use detected CPU features
- [ ] Benchmark JIT vs handwritten vectorized

**JIT Specialization Strategy**:

For shape [1×2048]@[2048×2048] with AVX2:
```llvm
; Generated LLVM IR for matmul_1x2048_2048x2048_avx2()

define void @matmul_specialized(float* %C, float* %A, float* %B) {
entry:
  ; Unrolled loop for j (2048 iterations, 8 at a time = 256 iterations)
  %j_0 = getelementptr float, float* %B, i64 0
  %v_b_0 = call <8 x float> @llvm.x86.avx.loadu.ps.256(i8* %j_0)

  ; ... unrolled K loop with FMA ...

  call void @llvm.x86.avx.storeu.ps.256(i8* %C, <8 x float> %result)
  ret void
}
```

**JIT Code Generation**:
```c
// kernel_lib/jit/matmul_jit.c

void* jit_compile_matmul(int M, int N, int K, CpuFeatures features) {
    // 1. Create LLVM module
    LLVMModuleRef mod = LLVMModuleCreateWithName("matmul_specialized");

    // 2. Generate specialized function
    //    - Hardcode M, N, K constants
    //    - Unroll loops based on dimensions
    //    - Use AVX2 intrinsics if available

    // 3. JIT compile at O3
    LLVMExecutionEngineRef engine;
    LLVMCreateExecutionEngineForModule(&engine, mod, &error);
    void* func = LLVMGetPointerToGlobal(engine, specialized_func);

    return func;
}
```

**Expected Performance**:
```
Handwritten AVX2:      15 ms/token
JIT specialized AVX2:  10 ms/token   (1.5× additional speedup from unrolling)
```

**Deliverables**:
- JIT code generator for matrix multiply
- Specialized functions for hot shapes
- Benchmark comparison
- JIT compilation time measurement

---

### Session 37: Layer Fusion & Optimization

**Goals**:
- [ ] Fuse attention operations (Q/K/V in one pass)
- [ ] Optimize memory layout (AoS → SoA)
- [ ] Eliminate temporary allocations
- [ ] Profile optimized inference

**Fusion Opportunities**:

**Before** (3 separate matrix multiplies):
```c
float* Q = matmul(hidden, W_q);  // [seq × hidden] @ [hidden × hidden]
float* K = matmul(hidden, W_k);  // [seq × hidden] @ [hidden × hidden]
float* V = matmul(hidden, W_v);  // [seq × hidden] @ [hidden × hidden]
```

**After** (fused, single kernel loop):
```c
void qkv_fused(float* Q, float* K, float* V, const float* hidden,
               const float* W_q, const float* W_k, const float* W_v,
               int seq_len, int hidden_size) {
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < hidden_size; j += 8) {
            __m256 h = _mm256_loadu_ps(&hidden[i*hidden_size + j]);

            // Q projection (unrolled)
            __m256 q = _mm256_setzero_ps();
            for (int k = 0; k < hidden_size; k++) {
                q = _mm256_fmadd_ps(h, _mm256_broadcast_ss(&W_q[k*hidden_size + j]), q);
            }
            _mm256_storeu_ps(&Q[i*hidden_size + j], q);

            // K projection (same loop, different weights)
            __m256 k = _mm256_setzero_ps();
            // ... similar ...

            // V projection
            __m256 v = _mm256_setzero_ps();
            // ... similar ...
        }
    }
}
```

**Memory Layout Optimization**:

**Before** (AoS - Array of Structures):
```c
struct Token {
    float embedding[2048];
    float q[2048];
    float k[2048];
    float v[2048];
};
Token tokens[seq_len];  // Bad cache locality
```

**After** (SoA - Structure of Arrays):
```c
struct Activations {
    float* embeddings;  // [seq_len × 2048] - contiguous
    float* q;           // [seq_len × 2048] - contiguous
    float* k;           // [seq_len × 2048] - contiguous
    float* v;           // [seq_len × 2048] - contiguous
};
```

**Expected Performance**:
```
Before fusion:         10 ms/token
After fusion + SoA:    5-7 ms/token  (1.5-2× speedup)
```

**Deliverables**:
- Fused QKV kernel
- SoA memory layout
- Eliminated temp allocations
- Performance measurements

---

### Session 38: Convergence & Native Export

**Goals**:
- [ ] Profile hot code paths (LLVM + model)
- [ ] Identify dead code in LLVM
- [ ] Generate native export (O3 optimized)
- [ ] Measure final binary size

**Convergence Strategy** (from "Grow to Shrink"):

```
Boot 1:    [~130 MB] Full LLVM + TinyLlama weights
           → Interpreted (slow, profile everything)

Boot 100:  [~80 MB]  JIT compiled matmul + attention
           → 5-10 ms/token

Boot 500:  [~40 MB]  Dead LLVM code eliminated
           → Specialized kernels only

Boot 1000: [~10 MB]  Pure native export
           → No LLVM runtime, pure machine code
```

**Native Export Process**:

1. **Identify used LLVM components**:
```bash
nm kernel.elf | grep " T " | grep llvm | wc -l
# Used: ~200 LLVM symbols (vs 32,000 total)
```

2. **Extract compiled functions**:
```c
void* matmul_specialized = /* JIT compiled */;
void* qkv_fused = /* JIT compiled */;
void* ffn_fused = /* JIT compiled */;
```

3. **Generate native binary**:
```bash
# Compile model.c with JIT-generated .o files
clang -O3 model.c matmul_specialized.o qkv_fused.o -o tinyllama_native
```

4. **Remove LLVM dependency**:
```bash
ldd tinyllama_native
# No libLLVM.so! Pure native code
```

**Expected Sizes**:
```
Full system:           130 MB (LLVM + weights)
Native export:         10 MB (weights + specialized code)
Reduction:             13× smaller
```

**Deliverables**:
- Native export tool
- Convergence measurements
- Final binary size analysis
- Performance validation

---

### Session 39: Persistence & Snapshots

**Goals**:
- [ ] Implement FAT16 write support
- [ ] Save optimized code to disk
- [ ] Load snapshot on next boot
- [ ] Demonstrate convergence loop

**Snapshot Format**:

```c
struct Snapshot {
    uint32_t magic;              // 0x534E4150 ("SNAP")
    uint32_t version;            // 1
    uint32_t boot_count;
    uint64_t timestamp;

    // Optimized functions
    uint32_t num_functions;
    struct {
        char name[64];           // "matmul_1x2048_2048x2048"
        uint32_t code_size;
        uint8_t code[];          // Machine code
    } functions[];

    // Profiling data
    uint32_t num_profiles;
    MatMulProfile profiles[];
};
```

**Snapshot Workflow**:

```c
// Boot N
void boot_main(void) {
    // 1. Load snapshot (if exists)
    Snapshot* snap = load_snapshot("bareflow.snap");

    if (snap && snap->boot_count < 1000) {
        // 2. Use pre-compiled functions
        install_optimized_functions(snap);

        // 3. Continue profiling
        run_inference_with_profiling();

        // 4. JIT compile new hot paths
        optimize_new_hot_functions();

        // 5. Save updated snapshot
        snap->boot_count++;
        save_snapshot(snap, "bareflow.snap");
    } else {
        // 6. Converged! Pure native mode
        run_inference_native_only();
    }
}
```

**Convergence Detection**:
```c
bool has_converged(Snapshot* snap) {
    // Converged if:
    // - Boot count > 500
    // - No new hot functions in last 100 boots
    // - Performance stable (< 1% variance)
    return snap->boot_count > 500 &&
           snap->new_functions_last_100 == 0 &&
           snap->perf_variance < 0.01;
}
```

**Deliverables**:
- FAT16 write implementation
- Snapshot save/load
- Convergence detection
- Demonstration: Boot 1 → Boot 1000

---

## Success Metrics - Phase 5

### Performance

| Metric | Target | Measurement |
|--------|--------|-------------|
| **Inference speed** | ≤1 second/token | Profile with rdtsc |
| **Memory usage** | ≤256 MB total | Peak heap usage |
| **Boot time** | ≤5 seconds | QEMU boot to first token |
| **JIT compile time** | ≤100 ms per function | LLVM OrcJIT overhead |

### Size

| Metric | Target | Measurement |
|--------|--------|-------------|
| **Boot 1** | ~130 MB | LLVM + weights |
| **Boot 100** | ~80 MB | Partial optimization |
| **Boot 500** | ~40 MB | Dead code eliminated |
| **Boot 1000** | ~10 MB | Pure native export |

### Optimization

| Metric | Target | Measurement |
|--------|--------|-------------|
| **Speedup (scalar → SSE2)** | 2-3× | Benchmark matmul |
| **Speedup (SSE2 → AVX2)** | 2-3× | Benchmark matmul |
| **Speedup (AVX2 → JIT)** | 1.5-2× | Benchmark matmul |
| **Overall speedup** | 10-15× | End-to-end inference |

### Convergence

| Metric | Target | Measurement |
|--------|--------|-------------|
| **Convergence time** | ≤500 boots | Snapshot analysis |
| **Final binary size** | ≤10 MB | Native export |
| **Performance stability** | <1% variance | Last 100 boots |

---

## Risks & Mitigations

### Risk 1: Paging Implementation Complex
**Probability**: Medium
**Impact**: High (blocks all of Phase 5)
**Mitigation**:
- Start with simple identity mapping
- Use QEMU monitor to inspect page tables
- Implement page fault handler for debugging
- Reference: Intel SDM Volume 3A Chapter 4

### Risk 2: TinyLlama Weights Too Large
**Probability**: Low
**Impact**: Medium (need quantization)
**Mitigation**:
- Start with smaller model (e.g., TinyStories-33M)
- Implement int8 quantization (4× size reduction)
- Consider model pruning (remove unused layers)

### Risk 3: JIT Compilation Too Slow
**Probability**: Medium
**Impact**: Medium (hurts boot time)
**Mitigation**:
- Cache compiled functions (snapshots)
- Use O1 instead of O3 (faster compilation)
- Lazy compilation (only compile on N-th call)

### Risk 4: Memory Fragmentation
**Probability**: Medium
**Impact**: Medium (malloc failures)
**Mitigation**:
- Implement defragmentation in malloc_llvm.c
- Use slab allocator for fixed-size objects
- Pre-allocate large buffers (weights, activations)

### Risk 5: Convergence Takes Too Long
**Probability**: Low
**Impact**: Low (just more boots needed)
**Mitigation**:
- Aggressive initial profiling (sample every call)
- Pre-optimize common shapes (2048×2048)
- Share profiles across boots (cumulative)

---

## Dependencies

### External Dependencies
- LLVM 18.1.8 (118 MB - already validated)
- TinyLlama weights (~60 MB - to download)
- QEMU x86-64 (for testing)

### Internal Dependencies
- kernel_lib_llvm.a (29 KB - ✅ complete)
- Paging implementation (Session 31 - CRITICAL)
- malloc_llvm.c working (depends on paging)
- Serial I/O for debugging (✅ complete)

### Optional Dependencies
- FAT16 filesystem (for persistence) - can defer
- VGA graphics (for visualization) - nice-to-have
- Keyboard input (for interactive mode) - future

---

## Tools & References

### Documentation
- Intel SDM Volume 3A: x86-64 paging
- LLVM OrcJIT Tutorial: https://llvm.org/docs/ORCv2.html
- TinyLlama paper: https://arxiv.org/abs/2401.02385
- AVX2 intrinsics: https://www.intel.com/content/www/us/en/docs/intrinsics-guide

### Code Examples
- Phase 3 tests (userspace LLVM validation)
- Phase 4 tests (bare-metal QEMU kernel)
- malloc investigation (paging insights)

### Measurement Tools
- rdtsc (cycle counting)
- QEMU monitor (page table inspection)
- nm (symbol analysis)
- objdump (disassembly)

---

## Phase 5 Deliverables

### Code
1. **Paging implementation** (boot.S) - ~200 lines
2. **Weight loader** (kernel_lib/model/) - ~300 lines
3. **TinyLlama model** (tinyllama/model.c) - ~800 lines
4. **Matrix multiply JIT** (kernel_lib/jit/) - ~400 lines
5. **Snapshot system** (kernel_lib/persist/) - ~500 lines

### Documentation
1. **Session summaries** (31-39) - 9 documents
2. **Paging guide** (how to setup page tables) - 1 document
3. **JIT specialization guide** (matrix multiply optimization) - 1 document
4. **Convergence analysis** (boot progression) - 1 document

### Tests
1. **Paging tests** (identity mapping, heap access) - 3 tests
2. **Weight loader tests** (format validation) - 2 tests
3. **Inference tests** (forward pass, token generation) - 4 tests
4. **JIT tests** (specialization, performance) - 5 tests
5. **Convergence tests** (snapshot save/load) - 3 tests

**Total**: ~2200 lines of code, ~12 documents, ~17 tests

---

## Timeline

```
Week 1 (Sessions 31-33):
  Session 31: Paging implementation (2-3 days)
  Session 32: Weight format design (1 day)
  Session 33: Model loading (1-2 days)

Week 2 (Sessions 34-36):
  Session 34: Matrix profiling (1 day)
  Session 35: CPU detection + vectorization (2 days)
  Session 36: JIT specialization (2 days)

Week 3 (Sessions 37-39):
  Session 37: Layer fusion (2 days)
  Session 38: Native export (2 days)
  Session 39: Persistence (1-2 days)

Week 4 (Buffer):
  Documentation finalization
  Integration testing
  Performance validation
```

**Total Duration**: 3-4 weeks (depending on complexity)

---

## Next Steps

**Immediate**: Session 31 - Paging Implementation

**Actions**:
1. Read Intel SDM Volume 3A Chapter 4 (paging)
2. Design page table layout (PML4 → PDPT → PD → PT)
3. Implement init_paging() in boot.S
4. Test malloc with paging enabled
5. Document results in SESSION_31_SUMMARY.md

**Success Criteria**:
- malloc(1024) works without triple fault
- malloc(1 MB) works
- malloc(10 MB) works
- free() and coalescing work
- Documented in QEMU

---

**Phase 5 Status**: 📝 READY TO START
**Critical Path**: Paging → Model Loading → JIT Optimization → Convergence
**Expected Completion**: Session 39 (~3-4 weeks)

---

*"Le programme se suffit à lui-même, embarque tout au départ, s'auto-profile, s'auto-optimise, et converge vers un binaire minimal."*

**BareFlow - Self-Optimizing Unikernel**
**Phase 5**: 🚀 READY - Starting Session 31 (Paging Implementation)
