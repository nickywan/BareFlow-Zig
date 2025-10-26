/**
 * TinyLlama Inference Functions
 *
 * Pure C implementation of transformer inference for bare-metal environment.
 * Optimized for minimal memory footprint and no dependencies.
 */

#ifndef TINYLLAMA_INFERENCE_H
#define TINYLLAMA_INFERENCE_H

#include "tinyllama_model.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Math Utilities
// ============================================================================

/**
 * Fast approximation of sqrt (good enough for RMSNorm)
 */
float fast_sqrt(float x);

/**
 * Fast approximation of exp (for softmax)
 */
float fast_exp(float x);

// ============================================================================
// Normalization
// ============================================================================

/**
 * RMS Normalization (Root Mean Square Layer Normalization)
 *
 * Normalizes input vector using RMS instead of standard deviation.
 * Formula: x_norm = x * scale / sqrt(mean(x^2) + eps)
 *
 * @param x Input/output vector [size]
 * @param weight Learned weights [size]
 * @param size Vector dimension
 */
void rms_norm(float* x, const float* weight, uint32_t size);

// ============================================================================
// Position Encoding
// ============================================================================

/**
 * Rotary Position Embeddings (RoPE)
 *
 * Applies rotary position embeddings to query and key vectors.
 * This encodes position information without adding extra parameters.
 *
 * @param q Query vector [n_heads, head_dim]
 * @param k Key vector [n_heads, head_dim]
 * @param pos Position index in sequence
 * @param n_heads Number of attention heads
 * @param head_dim Dimension per head
 */
void rope_encoding(float* q, float* k, uint32_t pos, uint32_t n_heads, uint32_t head_dim);

// ============================================================================
// Attention
// ============================================================================

/**
 * Softmax activation
 *
 * Converts logits to probabilities: softmax(x_i) = exp(x_i) / sum(exp(x))
 * Uses numerically stable implementation with max subtraction.
 *
 * @param x Input/output vector [size]
 * @param size Vector dimension
 */
void softmax(float* x, uint32_t size);

/**
 * Multi-head self-attention
 *
 * Core attention mechanism: Attention(Q,K,V) = softmax(QK^T/sqrt(d))V
 *
 * @param x Input activations [hidden_size]
 * @param wq Query weights (quantized)
 * @param wk Key weights (quantized)
 * @param wv Value weights (quantized)
 * @param wo Output weights (quantized)
 * @param pos Position in sequence
 * @param n_heads Number of attention heads
 * @param hidden_size Model hidden dimension
 */
void attention(
    float* x,
    const QuantizedTensor* wq,
    const QuantizedTensor* wk,
    const QuantizedTensor* wv,
    const QuantizedTensor* wo,
    uint32_t pos,
    uint32_t n_heads,
    uint32_t hidden_size
);

// ============================================================================
// Feed-Forward Network
// ============================================================================

/**
 * SwiGLU activation function
 *
 * SwiGLU(x) = Swish(x) * Linear(x)
 * Where Swish(x) = x * sigmoid(x)
 *
 * Used in the feed-forward network for better performance than ReLU.
 *
 * @param x1 First linear transformation output
 * @param x2 Second linear transformation output (gate)
 * @param size Vector dimension
 */
void swiglu(float* x1, const float* x2, uint32_t size);

/**
 * Feed-forward network (FFN)
 *
 * Two-layer MLP with SwiGLU activation:
 * FFN(x) = W2(SwiGLU(W1(x)))
 *
 * @param x Input/output vector [hidden_size]
 * @param w1 First layer weights (quantized) [hidden, 4*hidden]
 * @param w2 Second layer weights (quantized) [4*hidden, hidden]
 * @param hidden_size Model hidden dimension
 */
void feed_forward(
    float* x,
    const QuantizedTensor* w1,
    const QuantizedTensor* w2,
    uint32_t hidden_size
);

// ============================================================================
// Matrix Operations
// ============================================================================

/**
 * Matrix-vector multiplication with INT8 quantized weights
 *
 * Computes: y = W * x
 * Where W is quantized to INT8 and x is float32.
 *
 * @param y Output vector [rows]
 * @param W Weight matrix (quantized) [rows, cols]
 * @param x Input vector [cols]
 */
void matmul_int8(float* y, const QuantizedTensor* W, const float* x);

/**
 * Element-wise vector addition: x = x + y
 */
void vec_add(float* x, const float* y, uint32_t size);

/**
 * Element-wise vector multiplication: x = x * y
 */
void vec_mul(float* x, const float* y, uint32_t size);

// ============================================================================
// Transformer Layer
// ============================================================================

/**
 * Single transformer block forward pass
 *
 * Implements the full transformer layer:
 * 1. x = x + Attention(RMSNorm(x))
 * 2. x = x + FFN(RMSNorm(x))
 *
 * @param x Input/output activations [hidden_size]
 * @param layer Transformer layer with all weights
 * @param pos Position in sequence
 */
void transformer_block(
    float* x,
    const TransformerLayer* layer,
    uint32_t pos
);

// ============================================================================
// Full Model Forward Pass
// ============================================================================

/**
 * Complete forward pass through TinyLlama model
 *
 * Takes input token and produces logits for next token prediction.
 *
 * Steps:
 * 1. Embed input token
 * 2. Pass through all transformer layers
 * 3. Apply final layer norm
 * 4. Project to vocabulary logits
 *
 * @param model TinyLlama model
 * @param token Input token ID
 * @param pos Position in sequence
 * @param logits Output logits [vocab_size]
 * @return 0 on success, -1 on error
 */
int tinyllama_forward_token(
    const TinyLlamaModel* model,
    uint32_t token,
    uint32_t pos,
    float* logits
);

#ifdef __cplusplus
}
#endif

#endif // TINYLLAMA_INFERENCE_H
