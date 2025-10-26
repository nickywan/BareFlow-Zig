/**
 * TinyLlama Model Implementation
 *
 * Provides model creation, weight loading, and inference stubs.
 */

#include "tinyllama_model.h"

// Basic definitions
#ifndef NULL
#define NULL ((void*)0)
#endif

// External malloc/free from kernel_lib
extern void* malloc(unsigned long size);
extern void free(void* ptr);

// Serial output for debugging
extern void serial_puts(const char* str);
extern void serial_put_uint(unsigned int value);

// ============================================================================
// Helper Functions
// ============================================================================

static void debug_print(const char* msg) {
    serial_puts(msg);
}

// ============================================================================
// Model Size Estimation
// ============================================================================

uint64_t tinyllama_estimate_size() {
    uint64_t total = 0;

    // Token embeddings: [vocab_size, hidden] in INT8
    total += (uint64_t)LLAMA_VOCAB_SIZE * LLAMA_HIDDEN_SIZE;

    // Each transformer layer
    uint64_t layer_size = 0;
    // Attention matrices (Q, K, V, O): 4 × [hidden, hidden] in INT8
    layer_size += 4 * (uint64_t)LLAMA_HIDDEN_SIZE * LLAMA_HIDDEN_SIZE;
    // FFN matrices: [hidden, 4*hidden] + [4*hidden, hidden] in INT8
    layer_size += (uint64_t)LLAMA_HIDDEN_SIZE * (4 * LLAMA_HIDDEN_SIZE);
    layer_size += (uint64_t)(4 * LLAMA_HIDDEN_SIZE) * LLAMA_HIDDEN_SIZE;
    // Layer norms: 4 × hidden × 4 bytes (float32)
    layer_size += 4 * LLAMA_HIDDEN_SIZE * 4;

    total += layer_size * LLAMA_N_LAYERS;

    // Final layer norm: 2 × hidden × 4 bytes
    total += 2 * LLAMA_HIDDEN_SIZE * 4;

    // Output projection: [hidden, vocab_size] in INT8
    total += (uint64_t)LLAMA_HIDDEN_SIZE * LLAMA_VOCAB_SIZE;

    return total;
}

// ============================================================================
// Quantized Tensor Allocation
// ============================================================================

static int alloc_quantized_tensor(QuantizedTensor* tensor, uint32_t rows, uint32_t cols) {
    tensor->rows = rows;
    tensor->cols = cols;
    tensor->scale = 0.01f;       // Default scale
    tensor->zero_point = 0;

    // Allocate INT8 data
    uint64_t size = (uint64_t)rows * cols;
    tensor->data = (int8_t*)malloc(size);

    if (!tensor->data) {
        return -1;  // Allocation failed
    }

    // Note: Not zero-initialized (memset causes issues in bare-metal)
    // Will be filled by tinyllama_load_weights()

    return 0;
}

static void free_quantized_tensor(QuantizedTensor* tensor) {
    if (tensor->data) {
        free(tensor->data);
        tensor->data = NULL;
    }
}

// ============================================================================
// Model Creation
// ============================================================================

TinyLlamaModel* tinyllama_create_model() {
    serial_puts("[TinyLlama] Creating model... ");

    // Allocate model structure
    TinyLlamaModel* model = (TinyLlamaModel*)malloc(sizeof(TinyLlamaModel));

    if (!model) {
        serial_puts("ERROR\n");
        return NULL;
    }

    serial_puts("OK\n");
    serial_puts("[TinyLlama] Config... ");

    // Set configuration
    model->n_layers = LLAMA_N_LAYERS;
    model->hidden_size = LLAMA_HIDDEN_SIZE;
    model->n_heads = LLAMA_N_HEADS;
    model->vocab_size = LLAMA_VOCAB_SIZE;
    model->max_seq_len = LLAMA_MAX_SEQ_LEN;
    serial_puts("OK\n");

    // Allocate token embeddings (SKIP for test - 65 MB too big for 32 MB heap)
    model->token_embeddings.data = NULL;
    model->token_embeddings.rows = 0;
    model->token_embeddings.cols = 0;

    serial_puts("[TinyLlama] Layers array... ");
    // Allocate transformer layers array
    model->layers = (TransformerLayer*)malloc(LLAMA_N_LAYERS * sizeof(TransformerLayer));
    if (!model->layers) {
        serial_puts("ERROR\n");
        free(model);
        return NULL;
    }
    serial_puts("OK\n");

    // Allocate layers (TEST MODE: 1 complete layer - ~48 MB)
    uint32_t test_layers = 1;
    serial_puts("[TinyLlama] Allocating 1 complete layer... ");

    for (uint32_t i = 0; i < test_layers && i < LLAMA_N_LAYERS; i++) {
        TransformerLayer* layer = &model->layers[i];

        // Allocate attention matrices (Q, K, V, O)
        if (alloc_quantized_tensor(&layer->wq, LLAMA_HIDDEN_SIZE, LLAMA_HIDDEN_SIZE) != 0 ||
            alloc_quantized_tensor(&layer->wk, LLAMA_HIDDEN_SIZE, LLAMA_HIDDEN_SIZE) != 0 ||
            alloc_quantized_tensor(&layer->wv, LLAMA_HIDDEN_SIZE, LLAMA_HIDDEN_SIZE) != 0 ||
            alloc_quantized_tensor(&layer->wo, LLAMA_HIDDEN_SIZE, LLAMA_HIDDEN_SIZE) != 0) {
            serial_puts("ERROR (attention)\n");
            free(model->layers);
            free(model);
            return NULL;
        }

        // Allocate feed-forward matrices (W1, W2)
        if (alloc_quantized_tensor(&layer->w1, LLAMA_HIDDEN_SIZE, 4 * LLAMA_HIDDEN_SIZE) != 0 ||
            alloc_quantized_tensor(&layer->w2, 4 * LLAMA_HIDDEN_SIZE, LLAMA_HIDDEN_SIZE) != 0) {
            serial_puts("ERROR (FFN)\n");
            free(model->layers);
            free(model);
            return NULL;
        }

        // Allocate layer norm weights (float32)
        layer->ln1_weight = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));
        layer->ln1_bias = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));
        layer->ln2_weight = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));
        layer->ln2_bias = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));

        if (!layer->ln1_weight || !layer->ln1_bias || !layer->ln2_weight || !layer->ln2_bias) {
            serial_puts("ERROR (layer norm)\n");
            free(model->layers);
            free(model);
            return NULL;
        }
    }

    serial_puts("OK\n");

    // Zero out remaining layers
    for (uint32_t i = test_layers; i < LLAMA_N_LAYERS; i++) {
        TransformerLayer* layer = &model->layers[i];
        layer->wq.data = NULL;
        layer->wk.data = NULL;
        layer->wv.data = NULL;
        layer->wo.data = NULL;
        layer->w1.data = NULL;
        layer->w2.data = NULL;
        layer->ln1_weight = NULL;
        layer->ln1_bias = NULL;
        layer->ln2_weight = NULL;
        layer->ln2_bias = NULL;
    }

    serial_puts("[TinyLlama] Final LN... ");
    // Allocate final layer norm (float32 arrays)
    model->final_ln_weight = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));
    model->final_ln_bias = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));

    if (!model->final_ln_weight || !model->final_ln_bias) {
        serial_puts("ERROR\n");
        free(model->layers);
        free(model);
        return NULL;
    }
    serial_puts("OK\n");

    // Allocate output projection (SKIP for test - also 65 MB)
    model->output.data = NULL;
    model->output.rows = 0;
    model->output.cols = 0;

    serial_puts("[TinyLlama] Complete!\n");
    return model;
}

// ============================================================================
// Model Destruction
// ============================================================================

void tinyllama_free_model(TinyLlamaModel* model) {
    if (!model) return;

    // Free token embeddings
    free_quantized_tensor(&model->token_embeddings);

    // Free layers
    if (model->layers) {
        for (uint32_t i = 0; i < model->n_layers; i++) {
            TransformerLayer* layer = &model->layers[i];
            free_quantized_tensor(&layer->wq);
            free_quantized_tensor(&layer->wk);
            free_quantized_tensor(&layer->wv);
            free_quantized_tensor(&layer->wo);
            free_quantized_tensor(&layer->w1);
            free_quantized_tensor(&layer->w2);
            if (layer->ln1_weight) free(layer->ln1_weight);
            if (layer->ln1_bias) free(layer->ln1_bias);
            if (layer->ln2_weight) free(layer->ln2_weight);
            if (layer->ln2_bias) free(layer->ln2_bias);
        }
        free(model->layers);
    }

    // Free final layer norm
    if (model->final_ln_weight) free(model->final_ln_weight);
    if (model->final_ln_bias) free(model->final_ln_bias);

    // Free output projection
    free_quantized_tensor(&model->output);

    // Free model structure itself
    free(model);
}

// ============================================================================
// Weight Loading
// ============================================================================

int tinyllama_load_weights(TinyLlamaModel* model) {
    if (!model) return -1;

    serial_puts("[TinyLlama] Loading weights... ");

    // Initialize with dummy values (constant 1)
    // Real implementation would load from binary blob or disk

    for (uint32_t layer_idx = 0; layer_idx < model->n_layers; layer_idx++) {
        TransformerLayer* layer = &model->layers[layer_idx];

        // Initialize attention matrices
        if (layer->wq.data) {
            for (uint32_t i = 0; i < layer->wq.rows * layer->wq.cols; i++) {
                layer->wq.data[i] = 1;
            }
        }

        // Initialize feed-forward matrices
        if (layer->w1.data) {
            for (uint32_t i = 0; i < layer->w1.rows * layer->w1.cols; i++) {
                layer->w1.data[i] = 1;
            }
        }

        // Initialize layer norm weights
        if (layer->ln1_weight) {
            for (uint32_t i = 0; i < model->hidden_size; i++) {
                layer->ln1_weight[i] = 1.0f;
                layer->ln1_bias[i] = 0.0f;
            }
        }

        if (layer->ln2_weight) {
            for (uint32_t i = 0; i < model->hidden_size; i++) {
                layer->ln2_weight[i] = 1.0f;
                layer->ln2_bias[i] = 0.0f;
            }
        }
    }

    // Initialize final layer norm
    if (model->final_ln_weight) {
        for (uint32_t i = 0; i < model->hidden_size; i++) {
            model->final_ln_weight[i] = 1.0f;
            model->final_ln_bias[i] = 0.0f;
        }
    }

    serial_puts("OK\n");
    return 0;
}

// ============================================================================
// Inference (Stub)
// ============================================================================

int tinyllama_forward(TinyLlamaModel* model, uint32_t* tokens, uint32_t n_tokens) {
    (void)model;    // Unused for now
    (void)tokens;
    (void)n_tokens;

    debug_print("[TinyLlama] Forward pass (stub - not implemented yet)\n");
    return 0;
}
