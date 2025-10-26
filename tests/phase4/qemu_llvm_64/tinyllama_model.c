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
extern void* memset(void* s, int c, unsigned long n);

// Serial output for debugging
extern void serial_puts(const char* str);
extern void serial_put_uint(unsigned int value);

// ============================================================================
// Helper Functions
// ============================================================================

static void debug_print(const char* msg) {
    serial_puts(msg);
}

static void debug_print_uint(unsigned int value) {
    serial_put_uint(value);
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

    // Zero-initialize for testing
    memset(tensor->data, 0, size);

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
    debug_print("[TinyLlama] Creating model...\n");

    // Allocate model structure
    debug_print("[TinyLlama] About to malloc model struct\n");
    TinyLlamaModel* model = (TinyLlamaModel*)malloc(sizeof(TinyLlamaModel));
    debug_print("[TinyLlama] malloc returned\n");

    if (!model) {
        debug_print("[TinyLlama] ERROR: malloc() returned NULL\n");
        return NULL;
    }

    debug_print("[TinyLlama] malloc SUCCESS, setting config\n");

    // Set config one by one with debug
    debug_print("[TinyLlama] About to set n_layers\n");
    model->n_layers = LLAMA_N_LAYERS;
    debug_print("[TinyLlama] Set n_layers OK\n");

    debug_print("[TinyLlama] About to set hidden_size\n");
    model->hidden_size = LLAMA_HIDDEN_SIZE;
    debug_print("[TinyLlama] Set hidden_size OK\n");

    debug_print("[TinyLlama] About to set n_heads\n");
    model->n_heads = LLAMA_N_HEADS;
    debug_print("[TinyLlama] Set n_heads OK\n");

    debug_print("[TinyLlama] About to set vocab_size\n");
    model->vocab_size = LLAMA_VOCAB_SIZE;
    debug_print("[TinyLlama] Set vocab_size OK\n");

    debug_print("[TinyLlama] About to set max_seq_len\n");
    model->max_seq_len = LLAMA_MAX_SEQ_LEN;
    debug_print("[TinyLlama] Model config set\n");

    // Allocate token embeddings (SKIP for test - 65 MB too big for 32 MB heap)
    debug_print("[TinyLlama] Skipping token embeddings (65 MB - too large for test)\n");
    model->token_embeddings.data = NULL;
    model->token_embeddings.rows = 0;
    model->token_embeddings.cols = 0;

    // Allocate transformer layers
    debug_print("[TinyLlama] Allocating transformer layers...\n");

    model->layers = (TransformerLayer*)malloc(LLAMA_N_LAYERS * sizeof(TransformerLayer));
    if (!model->layers) {
        debug_print("[TinyLlama] ERROR: Failed to allocate layers array\n");
        free_quantized_tensor(&model->token_embeddings);
        free(model);
        return NULL;
    }

    // Allocate each layer
    // For testing with 32 MB heap, we only allocate FIRST LAYER as proof-of-concept
    uint32_t test_layers = 1;  // Only allocate 1 layer (4 MB) to fit in 32 MB heap
    debug_print("[TinyLlama] TEST MODE: Allocating only 1 layer (4 MB)\n");

    for (uint32_t i = 0; i < test_layers && i < LLAMA_N_LAYERS; i++) {
        TransformerLayer* layer = &model->layers[i];

        // Just allocate Q matrix for testing (full model would allocate all 6 matrices)
        if (alloc_quantized_tensor(&layer->wq, LLAMA_HIDDEN_SIZE, LLAMA_HIDDEN_SIZE) != 0) {
            debug_print("[TinyLlama] ERROR: Failed to allocate layer Q matrix\n");
            // TODO: cleanup previously allocated layers
            free(model->layers);
            free_quantized_tensor(&model->token_embeddings);
            free(model);
            return NULL;
        }
        debug_print("[TinyLlama] Layer allocated OK\n");

        // Set other pointers to NULL for now (not allocated in this test)
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

    // Allocate final layer norm (float32 arrays)
    model->final_ln_weight = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));
    model->final_ln_bias = (float*)malloc(LLAMA_HIDDEN_SIZE * sizeof(float));

    if (!model->final_ln_weight || !model->final_ln_bias) {
        debug_print("[TinyLlama] ERROR: Failed to allocate final layer norm\n");
        // TODO: full cleanup
        return NULL;
    }

    // Allocate output projection (SKIP for test - also 65 MB)
    debug_print("[TinyLlama] Skipping output projection (65 MB - too large for test)\n");
    model->output.data = NULL;
    model->output.rows = 0;
    model->output.cols = 0;

    debug_print("[TinyLlama] Model created successfully!\n");
    return model;
}

// ============================================================================
// Model Destruction
// ============================================================================

void tinyllama_free_model(TinyLlamaModel* model) {
    if (!model) return;

    debug_print("[TinyLlama] Freeing model...\n");

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

    debug_print("[TinyLlama] Model freed.\n");
}

// ============================================================================
// Weight Loading (Stub)
// ============================================================================

int tinyllama_load_weights(TinyLlamaModel* model) {
    if (!model) return -1;

    debug_print("[TinyLlama] Loading weights...\n");

    // For Phase 4, just initialize with dummy values
    // Real implementation would load from binary blob or disk

    debug_print("[TinyLlama] Weights loaded (dummy data).\n");
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
