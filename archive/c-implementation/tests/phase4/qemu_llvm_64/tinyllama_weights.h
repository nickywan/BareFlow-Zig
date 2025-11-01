/**
 * TinyLlama Weight Loading & Initialization
 *
 * Provides functions to load and initialize model weights.
 * For now, generates dummy weights for testing the inference pipeline.
 */

#ifndef TINYLLAMA_WEIGHTS_H
#define TINYLLAMA_WEIGHTS_H

#include "tinyllama_model.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Weight Initialization
// ============================================================================

/**
 * Initialize a quantized tensor with dummy data
 *
 * Fills the tensor with simple test pattern for validation.
 *
 * @param tensor Pointer to quantized tensor to initialize
 * @param rows Number of rows
 * @param cols Number of columns
 * @param seed Random seed for variation
 * @return 0 on success, -1 on error
 */
int init_quantized_tensor_dummy(
    QuantizedTensor* tensor,
    uint32_t rows,
    uint32_t cols,
    uint32_t seed
);

/**
 * Initialize a single transformer layer with dummy weights
 *
 * Allocates and initializes all weight matrices for one layer.
 *
 * @param layer Pointer to transformer layer
 * @param hidden_size Model hidden dimension
 * @param seed Random seed for variation
 * @return 0 on success, -1 on error
 */
int init_layer_weights_dummy(
    TransformerLayer* layer,
    uint32_t hidden_size,
    uint32_t seed
);

/**
 * Initialize entire TinyLlama model with dummy weights
 *
 * This function:
 * 1. Allocates all weight tensors
 * 2. Fills them with test data
 * 3. Initializes layer normalization weights
 *
 * @param model Pointer to TinyLlama model (structure already allocated)
 * @return 0 on success, -1 on error
 */
int init_model_weights_dummy(TinyLlamaModel* model);

/**
 * Initialize float weight vector with simple pattern
 *
 * Used for layer norm weights (RMSNorm)
 *
 * @param weights Float array to initialize
 * @param size Number of elements
 * @param value Initial value (typically 1.0 for layer norm)
 */
void init_float_weights(float* weights, uint32_t size, float value);

// ============================================================================
// Future: Real Weight Loading
// ============================================================================

/**
 * Load weights from binary file (NOT YET IMPLEMENTED)
 *
 * Will be implemented in future session to load real TinyLlama weights.
 *
 * @param model Pointer to TinyLlama model
 * @param weight_file_path Path to weight file
 * @return 0 on success, -1 on error
 */
int load_model_weights_from_file(TinyLlamaModel* model, const char* weight_file_path);

/**
 * Free all allocated weight memory
 *
 * Cleans up all dynamically allocated weight tensors.
 *
 * @param model Pointer to TinyLlama model
 */
void free_model_weights(TinyLlamaModel* model);

#ifdef __cplusplus
}
#endif

#endif // TINYLLAMA_WEIGHTS_H
