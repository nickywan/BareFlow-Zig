/**
 * TinyLlama Inference Implementation
 *
 * Bare-metal C implementation - no stdlib, optimized for -O0
 */

#include "tinyllama_inference.h"

// ============================================================================
// Math Utilities (Fast Approximations for Bare-Metal)
// ============================================================================

float fast_sqrt(float x) {
    // Newton-Raphson iteration for sqrt
    if (x <= 0.0f) return 0.0f;
    float guess = x;
    for (int i = 0; i < 5; i++) {
        guess = 0.5f * (guess + x / guess);
    }
    return guess;
}

float fast_exp(float x) {
    // Fast exp approximation using Taylor series (limited range)
    // Good enough for softmax with normalization
    if (x < -10.0f) return 0.0f;
    if (x > 10.0f) return 22026.0f; // e^10

    float result = 1.0f + x;
    float term = x;
    for (int i = 2; i < 6; i++) {
        term *= x / (float)i;
        result += term;
    }
    return result;
}

// ============================================================================
// Vector Operations
// ============================================================================

void vec_add(float* x, const float* y, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        x[i] += y[i];
    }
}

void vec_mul(float* x, const float* y, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        x[i] *= y[i];
    }
}

// ============================================================================
// Normalization
// ============================================================================

void rms_norm(float* x, const float* weight, uint32_t size) {
    // Calculate RMS
    float sum_sq = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        sum_sq += x[i] * x[i];
    }
    float rms = fast_sqrt(sum_sq / (float)size + 1e-6f);

    // Normalize and scale
    float inv_rms = 1.0f / rms;
    for (uint32_t i = 0; i < size; i++) {
        x[i] = x[i] * inv_rms * weight[i];
    }
}

// ============================================================================
// Softmax
// ============================================================================

void softmax(float* x, uint32_t size) {
    // Find max for numerical stability
    float max_val = x[0];
    for (uint32_t i = 1; i < size; i++) {
        if (x[i] > max_val) max_val = x[i];
    }

    // Compute exp and sum
    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        x[i] = fast_exp(x[i] - max_val);
        sum += x[i];
    }

    // Normalize
    float inv_sum = 1.0f / sum;
    for (uint32_t i = 0; i < size; i++) {
        x[i] *= inv_sum;
    }
}

// ============================================================================
// Matrix Operations
// ============================================================================

void matmul_int8(float* y, const QuantizedTensor* W, const float* x) {
    // y = W * x where W is INT8 quantized
    for (uint32_t i = 0; i < W->rows; i++) {
        float sum = 0.0f;
        for (uint32_t j = 0; j < W->cols; j++) {
            int8_t w_ij = W->data[i * W->cols + j];
            float w_float = ((float)w_ij - (float)W->zero_point) * W->scale;
            sum += w_float * x[j];
        }
        y[i] = sum;
    }
}

// ============================================================================
// RoPE (Rotary Position Embeddings)
// ============================================================================

void rope_encoding(float* q, float* k, uint32_t pos, uint32_t n_heads, uint32_t head_dim) {
    // Simplified RoPE - rotate pairs of dimensions
    float theta_base = 10000.0f;

    for (uint32_t h = 0; h < n_heads; h++) {
        for (uint32_t d = 0; d < head_dim; d += 2) {
            uint32_t idx = h * head_dim + d;

            // Compute rotation angle
            float freq = 1.0f / fast_exp((float)d * 0.693f / (float)head_dim * 9.21f); // log(10000)
            float theta = (float)pos * freq;

            // Approximate sin/cos with small angle if pos is small
            float cos_theta = 1.0f - theta * theta * 0.5f; // cos ≈ 1 - θ²/2
            float sin_theta = theta;  // sin ≈ θ

            // Rotate Q
            float q0 = q[idx];
            float q1 = q[idx + 1];
            q[idx] = q0 * cos_theta - q1 * sin_theta;
            q[idx + 1] = q0 * sin_theta + q1 * cos_theta;

            // Rotate K
            float k0 = k[idx];
            float k1 = k[idx + 1];
            k[idx] = k0 * cos_theta - k1 * sin_theta;
            k[idx + 1] = k0 * sin_theta + k1 * cos_theta;
        }
    }
}

// ============================================================================
// Attention (Simplified for Single Token)
// ============================================================================

void attention(
    float* x,
    const QuantizedTensor* wq,
    const QuantizedTensor* wk,
    const QuantizedTensor* wv,
    const QuantizedTensor* wo,
    uint32_t pos,
    uint32_t n_heads,
    uint32_t hidden_size
) {
    // Allocate temporary buffers (will use malloc from our bump allocator)
    uint32_t head_dim = hidden_size / n_heads;

    // TODO: Implement full attention with KV cache
    // For now, stub implementation that just applies output projection

    extern void* malloc(uint32_t size);
    extern void free(void* ptr);

    float* temp = (float*)malloc(hidden_size * sizeof(float));
    if (!temp) return;

    // Just do output projection for now (placeholder)
    matmul_int8(temp, wo, x);

    // Copy back
    for (uint32_t i = 0; i < hidden_size; i++) {
        x[i] = temp[i];
    }

    free(temp);
}

// ============================================================================
// SwiGLU Activation
// ============================================================================

void swiglu(float* x1, const float* x2, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        // Swish(x) = x * sigmoid(x) ≈ x * (1 / (1 + exp(-x)))
        // For simplicity, use tanh approximation: sigmoid(x) ≈ 0.5 + 0.5*tanh(x/2)
        float x_val = x2[i];
        float sigmoid_approx = 0.5f + 0.5f * x_val / (1.0f + (x_val < 0 ? -x_val : x_val));
        x1[i] = x1[i] * (x_val * sigmoid_approx);
    }
}

// ============================================================================
// Feed-Forward Network
// ============================================================================

void feed_forward(
    float* x,
    const QuantizedTensor* w1,
    const QuantizedTensor* w2,
    uint32_t hidden_size
) {
    extern void* malloc(uint32_t size);
    extern void free(void* ptr);

    uint32_t ffn_dim = w1->rows; // Should be 4 * hidden_size

    // Allocate temporary buffers
    float* hidden1 = (float*)malloc(ffn_dim * sizeof(float));
    float* hidden2 = (float*)malloc(ffn_dim * sizeof(float));
    if (!hidden1 || !hidden2) {
        if (hidden1) free(hidden1);
        if (hidden2) free(hidden2);
        return;
    }

    // First linear: hidden = W1 * x
    matmul_int8(hidden1, w1, x);

    // For SwiGLU, we need two separate transformations
    // Simplified: just use same hidden for both (not correct but works as placeholder)
    for (uint32_t i = 0; i < ffn_dim; i++) {
        hidden2[i] = hidden1[i];
    }

    // SwiGLU activation
    swiglu(hidden1, hidden2, ffn_dim);

    // Second linear: x = W2 * hidden
    matmul_int8(x, w2, hidden1);

    free(hidden1);
    free(hidden2);
}

// ============================================================================
// Transformer Block
// ============================================================================

void transformer_block(
    float* x,
    const TransformerLayer* layer,
    uint32_t pos
) {
    extern void* malloc(uint32_t size);
    extern void free(void* ptr);

    uint32_t hidden_size = LLAMA_HIDDEN_SIZE;
    uint32_t n_heads = LLAMA_N_HEADS;

    // Allocate residual buffer
    float* residual = (float*)malloc(hidden_size * sizeof(float));
    if (!residual) return;

    // Save residual
    for (uint32_t i = 0; i < hidden_size; i++) {
        residual[i] = x[i];
    }

    // Pre-norm: RMSNorm(x)
    rms_norm(x, layer->ln1_weight, hidden_size);

    // Attention
    attention(x, &layer->wq, &layer->wk, &layer->wv, &layer->wo,
              pos, n_heads, hidden_size);

    // Residual connection
    vec_add(x, residual, hidden_size);

    // Save residual again
    for (uint32_t i = 0; i < hidden_size; i++) {
        residual[i] = x[i];
    }

    // Pre-norm: RMSNorm(x)
    rms_norm(x, layer->ln2_weight, hidden_size);

    // Feed-forward
    feed_forward(x, &layer->w1, &layer->w2, hidden_size);

    // Residual connection
    vec_add(x, residual, hidden_size);

    free(residual);
}

// ============================================================================
// Full Forward Pass
// ============================================================================

int tinyllama_forward_token(
    const TinyLlamaModel* model,
    uint32_t token,
    uint32_t pos,
    float* logits
) {
    extern void* malloc(uint32_t size);
    extern void free(void* ptr);

    if (!model || !logits) return -1;
    if (token >= model->vocab_size) return -1;
    if (pos >= model->max_seq_len) return -1;

    uint32_t hidden_size = model->hidden_size;

    // Allocate activation buffer
    float* x = (float*)malloc(hidden_size * sizeof(float));
    if (!x) return -1;

    // 1. Token embedding
    for (uint32_t i = 0; i < hidden_size; i++) {
        int8_t emb = model->token_embeddings.data[token * hidden_size + i];
        x[i] = ((float)emb - (float)model->token_embeddings.zero_point)
               * model->token_embeddings.scale;
    }

    // 2. Pass through all transformer layers
    for (uint32_t layer_idx = 0; layer_idx < model->n_layers; layer_idx++) {
        transformer_block(x, &model->layers[layer_idx], pos);
    }

    // 3. Final layer norm
    rms_norm(x, model->final_ln_weight, hidden_size);

    // 4. Project to vocabulary
    matmul_int8(logits, &model->output, x);

    free(x);
    return 0;
}
