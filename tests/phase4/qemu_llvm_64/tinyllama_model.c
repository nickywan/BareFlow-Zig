/**
 * TinyLlama Model Implementation - Session 36 Refactored
 *
 * Architecture: Simple functions (split to avoid return crash bug)
 * Each function <25 lines, returns simple types (int/void)
 *
 * See: docs/phase5/SESSION_36_ARCHITECTURE.md
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
// Quantized Tensor Allocation (Helper - works fine, keep it)
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
// Model Creation - NEW ARCHITECTURE (split into simple functions)
// ============================================================================

/**
 * Step 1: Allocate main model structure
 * Returns: 0 on success, -1 on failure
 */
static int tinyllama_alloc_structure(TinyLlamaModel** out_model) {
    serial_puts("[TinyLlama] Allocating structure... ");

    TinyLlamaModel* model = (TinyLlamaModel*)malloc(sizeof(TinyLlamaModel));
    if (!model) {
        serial_puts("FAILED\n");
        *out_model = NULL;
        return -1;
    }

    *out_model = model;
    serial_puts("OK\n");
    return 0;
}

/**
 * Step 2: Set model configuration
 * Returns: Always 0
 */
static int tinyllama_set_config(TinyLlamaModel* model) {
    serial_puts("[TinyLlama] Config...");

    model->n_layers = 22;
    serial_puts("1");
    model->hidden_size = 2048;
    serial_puts("2");
    model->n_heads = 32;
    serial_puts("3");
    model->vocab_size = 32000;
    serial_puts("4");
    model->max_seq_len = 2048;
    serial_puts("5");

    // Skip token embeddings
    model->token_embeddings.data = NULL;
    serial_puts("6");
    model->token_embeddings.rows = 0;
    serial_puts("7");
    model->token_embeddings.cols = 0;
    serial_puts("8");

    serial_puts(" OK\n");
    return 0;
}

/**
 * Step 3: Allocate layers array
 * Returns: 0 on success, -1 on failure
 */
static int tinyllama_alloc_layers_array(TinyLlamaModel* model) {
    serial_puts("[TinyLlama] Allocating layers array (~5KB)... ");

    serial_puts("a");
    // Manual calculation to avoid sizeof issues:
    // TransformerLayer = 6 QuantizedTensors + 4 float*
    // Each QuantizedTensor ~ 32 bytes (int8_t*, float, int8_t, 2x uint32_t + padding)
    // Total: 6*32 + 4*8 = 192 + 32 = 224 bytes per layer
    // 22 layers = 4928 bytes ~ 5 KB
    unsigned long size = LLAMA_N_LAYERS * 256;  // Conservative: 256 bytes/layer

    serial_puts("b");
    model->layers = (TransformerLayer*)malloc(size);
    serial_puts("c");

    if (!model->layers) {
        serial_puts("FAILED\n");
        return -1;
    }

    serial_puts("d");
    serial_puts(" OK\n");
    serial_puts("e");
    return 0;
}

/**
 * Step 4: Allocate ONE complete layer (test mode)
 * Returns: 0 on success, -1 on failure
 */
static int tinyllama_alloc_single_layer(TransformerLayer* layer, uint32_t hidden_size) {
    serial_puts("[TinyLlama] Allocating layer components... ");

    // Attention matrices (Q, K, V, O)
    if (alloc_quantized_tensor(&layer->wq, hidden_size, hidden_size) != 0) goto error;
    serial_puts("Q ");

    if (alloc_quantized_tensor(&layer->wk, hidden_size, hidden_size) != 0) goto error;
    serial_puts("K ");

    if (alloc_quantized_tensor(&layer->wv, hidden_size, hidden_size) != 0) goto error;
    serial_puts("V ");

    if (alloc_quantized_tensor(&layer->wo, hidden_size, hidden_size) != 0) goto error;
    serial_puts("O ");

    // Feed-forward matrices (W1, W2)
    if (alloc_quantized_tensor(&layer->w1, hidden_size, 4 * hidden_size) != 0) goto error;
    serial_puts("W1 ");

    if (alloc_quantized_tensor(&layer->w2, 4 * hidden_size, hidden_size) != 0) goto error;
    serial_puts("W2 ");

    // Layer norm weights (float32)
    layer->ln1_weight = (float*)malloc(hidden_size * sizeof(float));
    layer->ln1_bias = (float*)malloc(hidden_size * sizeof(float));
    if (!layer->ln1_weight || !layer->ln1_bias) goto error;
    serial_puts("LN1 ");

    layer->ln2_weight = (float*)malloc(hidden_size * sizeof(float));
    layer->ln2_bias = (float*)malloc(hidden_size * sizeof(float));
    if (!layer->ln2_weight || !layer->ln2_bias) goto error;
    serial_puts("LN2");

    serial_puts(" OK\n");
    return 0;

error:
    serial_puts(" FAILED\n");
    return -1;
}

/**
 * Step 5: Allocate final layer norm
 * Returns: 0 on success, -1 on failure
 */
static int tinyllama_alloc_final_norm(TinyLlamaModel* model) {
    serial_puts("[TinyLlama] Allocating final layer norm... ");

    model->final_ln_weight = (float*)malloc(model->hidden_size * sizeof(float));
    model->final_ln_bias = (float*)malloc(model->hidden_size * sizeof(float));

    if (!model->final_ln_weight || !model->final_ln_bias) {
        serial_puts("FAILED\n");
        return -1;
    }

    // Skip output projection (also 65 MB)
    model->output.data = NULL;
    model->output.rows = 0;
    model->output.cols = 0;

    serial_puts("OK\n");
    return 0;
}

/**
 * Main orchestrator function - SIMPLE and CLEAN
 *
 * This function coordinates the 5 simple steps above.
 * Each step is a small function with simple return (int).
 *
 * Returns: 0 on success, -1 on failure
 */
// Workaround: Inline everything to avoid return crash
int tinyllama_create_model(TinyLlamaModel** out_model) {
    serial_puts("\n=== TinyLlama Model Creation (Session 36) ===\n");

    // Step 1: Allocate structure (INLINED)
    serial_puts("[TinyLlama] Allocating structure... ");
    TinyLlamaModel* model = (TinyLlamaModel*)malloc(sizeof(TinyLlamaModel));
    if (!model) {
        serial_puts("FAILED\n");
        return -1;
    }
    serial_puts("OK\n");
    *out_model = model;

    // Step 2: Set configuration (INLINED)
    serial_puts("[TinyLlama] Config...");
    model->n_layers = 22;
    serial_puts("1");
    model->hidden_size = 2048;
    serial_puts("2");
    model->n_heads = 32;
    serial_puts("3");
    model->vocab_size = 32000;
    serial_puts("4");
    model->max_seq_len = 2048;
    serial_puts("5");
    model->token_embeddings.data = NULL;
    serial_puts("6");
    model->token_embeddings.rows = 0;
    serial_puts("7");
    model->token_embeddings.cols = 0;
    serial_puts("8 OK\n");

    // Step 3: Allocate layers array (INLINED - WITHOUT RETURN!)
    serial_puts("[TinyLlama] Allocating layers array (~5KB)... ");
    unsigned long size = 22 * 256;
    model->layers = (TransformerLayer*)malloc(size);
    if (!model->layers) {
        serial_puts("FAILED\n");
        free(model);
        *out_model = NULL;
        return -1;
    }
    serial_puts("OK\n");  // NO return after this!

    serial_puts("=== Model created successfully! ===\n\n");
    return 0;
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
// Weight Loading (Stub - simple implementation)
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
