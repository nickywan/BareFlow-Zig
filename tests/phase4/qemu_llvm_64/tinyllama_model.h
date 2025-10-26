/**
 * TinyLlama Model Structures
 *
 * Lightweight structures for loading and running TinyLlama 1.1B model
 * in bare-metal environment (32 MB heap).
 *
 * Model details:
 * - Parameters: ~1.1B (FP16 = ~2.2 GB, quantized = ~550 MB)
 * - Layers: 22 transformer blocks
 * - Hidden size: 2048
 * - Attention heads: 32
 * - Vocab size: 32000
 *
 * For Phase 4, we'll use:
 * - INT8 quantization (8-bit weights)
 * - Minimal inference (no training)
 * - Embedded weights in binary (or load from disk later)
 */

#ifndef TINYLLAMA_MODEL_H
#define TINYLLAMA_MODEL_H

// ============================================================================
// Configuration
// ============================================================================

#define LLAMA_N_LAYERS      22      // Number of transformer layers
#define LLAMA_HIDDEN_SIZE   2048    // Hidden dimension
#define LLAMA_N_HEADS       32      // Number of attention heads
#define LLAMA_VOCAB_SIZE    32000   // Vocabulary size
#define LLAMA_MAX_SEQ_LEN   2048    // Maximum sequence length

// For INT8 quantization
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

// ============================================================================
// Model Structures
// ============================================================================

/**
 * Quantized weight tensor (INT8)
 *
 * For a weight of shape [M, N]:
 * - data: M × N int8_t values
 * - scale: Single float32 for de-quantization
 * - zero_point: Offset for asymmetric quantization
 *
 * De-quantization formula:
 *   float_value = (int8_value - zero_point) * scale
 */
typedef struct {
    int8_t*  data;        // Quantized weights (int8)
    float    scale;       // Scaling factor
    int8_t   zero_point;  // Zero point for asymmetric quantization
    uint32_t rows;        // Number of rows (M)
    uint32_t cols;        // Number of columns (N)
} QuantizedTensor;

/**
 * Single transformer layer
 *
 * Contains all weights for one transformer block:
 * - Attention weights (Q, K, V projections + output)
 * - Feed-forward weights (2 layers)
 * - Layer norm weights (2 sets)
 */
typedef struct {
    // Attention
    QuantizedTensor wq;    // Query projection    [hidden, hidden]
    QuantizedTensor wk;    // Key projection      [hidden, hidden]
    QuantizedTensor wv;    // Value projection    [hidden, hidden]
    QuantizedTensor wo;    // Output projection   [hidden, hidden]

    // Feed-forward
    QuantizedTensor w1;    // First FFN layer     [hidden, 4*hidden]
    QuantizedTensor w2;    // Second FFN layer    [4*hidden, hidden]

    // Layer normalization
    float* ln1_weight;     // Attention layer norm weights [hidden]
    float* ln1_bias;       // Attention layer norm bias    [hidden]
    float* ln2_weight;     // FFN layer norm weights       [hidden]
    float* ln2_bias;       // FFN layer norm bias          [hidden]
} TransformerLayer;

/**
 * Complete TinyLlama model
 *
 * Contains all weights and parameters for inference.
 */
typedef struct {
    // Token embedding
    QuantizedTensor token_embeddings;  // [vocab_size, hidden]

    // Transformer layers
    TransformerLayer* layers;          // Array of LLAMA_N_LAYERS layers

    // Final layer norm
    float* final_ln_weight;            // [hidden]
    float* final_ln_bias;              // [hidden]

    // Output projection (for logits)
    QuantizedTensor output;            // [hidden, vocab_size]

    // Model config
    uint32_t n_layers;
    uint32_t hidden_size;
    uint32_t n_heads;
    uint32_t vocab_size;
    uint32_t max_seq_len;
} TinyLlamaModel;

// ============================================================================
// Model Loading Functions
// ============================================================================

/**
 * Initialize model structure with allocated memory
 *
 * Allocates memory for all layers and weights, but doesn't load data yet.
 * Returns pointer to model, or NULL on allocation failure.
 */
TinyLlamaModel* tinyllama_create_model();

/**
 * Free all model memory
 */
void tinyllama_free_model(TinyLlamaModel* model);

/**
 * Load model weights from embedded data
 *
 * For Phase 4, we'll embed a small set of dummy weights in the binary.
 * Later phases will load from disk/network.
 *
 * Returns 0 on success, -1 on error.
 */
int tinyllama_load_weights(TinyLlamaModel* model);

/**
 * Get model size estimate (in bytes)
 *
 * Calculates total memory needed for INT8 quantized model.
 */
uint64_t tinyllama_estimate_size();

// ============================================================================
// Inference Functions (stub for now)
// ============================================================================

/**
 * Run forward pass on input tokens
 *
 * Stub for Phase 4 - we'll implement basic inference later.
 */
int tinyllama_forward(TinyLlamaModel* model, uint32_t* tokens, uint32_t n_tokens);

#endif // TINYLLAMA_MODEL_H
