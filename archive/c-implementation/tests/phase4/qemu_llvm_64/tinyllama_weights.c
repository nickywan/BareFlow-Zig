/**
 * TinyLlama Weight Loading & Initialization - Implementation
 *
 * For testing: generates simple dummy weights
 * Future: load real weights from binary file
 */

#include "tinyllama_weights.h"

// External malloc/free from bump allocator
extern void* malloc(uint32_t size);
extern void free(void* ptr);

// ============================================================================
// Simple PRNG for Weight Generation
// ============================================================================

// Linear congruential generator for simple random numbers
static uint32_t prng_state = 12345;

static void prng_seed(uint32_t seed) {
    prng_state = seed;
}

static uint32_t prng_next(void) {
    prng_state = prng_state * 1103515245 + 12345;
    return (prng_state / 65536) % 32768;
}

// Generate random int8 value in range [-127, 127]
static int8_t random_int8(void) {
    uint32_t r = prng_next();
    return (int8_t)((r % 255) - 127);
}

// ============================================================================
// Float Weight Initialization
// ============================================================================

void init_float_weights(float* weights, uint32_t size, float value) {
    for (uint32_t i = 0; i < size; i++) {
        weights[i] = value;
    }
}

// ============================================================================
// Quantized Tensor Initialization
// ============================================================================

int init_quantized_tensor_dummy(
    QuantizedTensor* tensor,
    uint32_t rows,
    uint32_t cols,
    uint32_t seed
) {
    if (!tensor) return -1;

    // Seed PRNG for reproducible weights
    prng_seed(seed);

    // Allocate data buffer
    uint32_t total_size = rows * cols;
    tensor->data = (int8_t*)malloc(total_size * sizeof(int8_t));
    if (!tensor->data) {
        return -1;
    }

    // Fill with random INT8 values
    for (uint32_t i = 0; i < total_size; i++) {
        tensor->data[i] = random_int8();
    }

    // Set quantization parameters
    tensor->rows = rows;
    tensor->cols = cols;
    tensor->scale = 0.01f;      // Simple scaling factor
    tensor->zero_point = 0;     // Symmetric quantization

    return 0;
}

// ============================================================================
// Layer Initialization
// ============================================================================

int init_layer_weights_dummy(
    TransformerLayer* layer,
    uint32_t hidden_size,
    uint32_t seed
) {
    if (!layer) return -1;

    uint32_t ffn_dim = 4 * hidden_size; // Feed-forward dimension

    // Initialize attention weights
    if (init_quantized_tensor_dummy(&layer->wq, hidden_size, hidden_size, seed + 1) != 0) {
        return -1;
    }
    if (init_quantized_tensor_dummy(&layer->wk, hidden_size, hidden_size, seed + 2) != 0) {
        return -1;
    }
    if (init_quantized_tensor_dummy(&layer->wv, hidden_size, hidden_size, seed + 3) != 0) {
        return -1;
    }
    if (init_quantized_tensor_dummy(&layer->wo, hidden_size, hidden_size, seed + 4) != 0) {
        return -1;
    }

    // Initialize feed-forward weights
    if (init_quantized_tensor_dummy(&layer->w1, ffn_dim, hidden_size, seed + 5) != 0) {
        return -1;
    }
    if (init_quantized_tensor_dummy(&layer->w2, hidden_size, ffn_dim, seed + 6) != 0) {
        return -1;
    }

    // Allocate and initialize layer norm weights
    layer->ln1_weight = (float*)malloc(hidden_size * sizeof(float));
    layer->ln2_weight = (float*)malloc(hidden_size * sizeof(float));
    if (!layer->ln1_weight || !layer->ln2_weight) {
        return -1;
    }

    init_float_weights(layer->ln1_weight, hidden_size, 1.0f);
    init_float_weights(layer->ln2_weight, hidden_size, 1.0f);

    return 0;
}

// ============================================================================
// Full Model Initialization
// ============================================================================

int init_model_weights_dummy(TinyLlamaModel* model) {
    if (!model) return -1;

    uint32_t hidden_size = model->hidden_size;
    uint32_t vocab_size = model->vocab_size;
    uint32_t n_layers = model->n_layers;

    // 1. Initialize token embeddings
    if (init_quantized_tensor_dummy(&model->token_embeddings, vocab_size, hidden_size, 1000) != 0) {
        return -1;
    }

    // 2. Initialize all transformer layers
    for (uint32_t i = 0; i < n_layers; i++) {
        // Use different seed for each layer
        if (init_layer_weights_dummy(&model->layers[i], hidden_size, 2000 + i * 100) != 0) {
            return -1;
        }
    }

    // 3. Initialize final layer norm
    model->final_ln_weight = (float*)malloc(hidden_size * sizeof(float));
    if (!model->final_ln_weight) {
        return -1;
    }
    init_float_weights(model->final_ln_weight, hidden_size, 1.0f);

    // 4. Initialize output projection (unembedding)
    if (init_quantized_tensor_dummy(&model->output, vocab_size, hidden_size, 9000) != 0) {
        return -1;
    }

    return 0;
}

// ============================================================================
// Weight Cleanup
// ============================================================================

static void free_quantized_tensor(QuantizedTensor* tensor) {
    if (tensor && tensor->data) {
        free(tensor->data);
        tensor->data = 0;
    }
}

static void free_layer_weights(TransformerLayer* layer) {
    if (!layer) return;

    free_quantized_tensor(&layer->wq);
    free_quantized_tensor(&layer->wk);
    free_quantized_tensor(&layer->wv);
    free_quantized_tensor(&layer->wo);
    free_quantized_tensor(&layer->w1);
    free_quantized_tensor(&layer->w2);

    if (layer->ln1_weight) {
        free(layer->ln1_weight);
        layer->ln1_weight = 0;
    }
    if (layer->ln2_weight) {
        free(layer->ln2_weight);
        layer->ln2_weight = 0;
    }
}

void free_model_weights(TinyLlamaModel* model) {
    if (!model) return;

    free_quantized_tensor(&model->token_embeddings);

    for (uint32_t i = 0; i < model->n_layers; i++) {
        free_layer_weights(&model->layers[i]);
    }

    if (model->final_ln_weight) {
        free(model->final_ln_weight);
        model->final_ln_weight = 0;
    }

    free_quantized_tensor(&model->output);
}

// ============================================================================
// Future: Real Weight Loading
// ============================================================================

int load_model_weights_from_file(TinyLlamaModel* model, const char* weight_file_path) {
    // TODO: Implement binary weight loading
    // For now, just return error
    (void)model;
    (void)weight_file_path;
    return -1;
}
