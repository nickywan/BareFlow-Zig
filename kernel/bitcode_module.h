// ============================================================================
// BAREFLOW - Bitcode Module Format
// ============================================================================
// File: kernel/bitcode_module.h
// Purpose: LLVM bitcode module format for runtime JIT compilation
// ============================================================================

#ifndef BITCODE_MODULE_H
#define BITCODE_MODULE_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// BITCODE MODULE FORMAT
// ============================================================================

#define BITCODE_MAGIC 0x4C4C4243  // "LLBC" (LLVM Bitcode)
#define PATTERN_MAGIC 0x50415454  // "PATT" (Pattern-based JIT)
#define MAX_BITCODE_NAME 32
#define MAX_ENTRY_NAME 64

// Bitcode module header (stored at offset 0 in .bc file)
typedef struct __attribute__((packed)) {
    uint32_t magic;                      // Must be BITCODE_MAGIC
    char module_name[MAX_BITCODE_NAME];  // Module name (e.g., "fibonacci")
    char entry_name[MAX_ENTRY_NAME];     // Entry function (e.g., "module_fibonacci")
    uint32_t bitcode_size;               // Size of LLVM bitcode
    uint32_t version;                    // Module version
    uint32_t opt_level;                  // Default optimization (0=O0, 1=O1, 2=O2, 3=O3)
    uint32_t reserved[2];                // Reserved for future use
    // Followed by: uint8_t bitcode_data[bitcode_size]
} bitcode_header_t;

// Bitcode module (header + data)
typedef struct {
    bitcode_header_t header;
    uint8_t* bitcode_data;  // LLVM IR bitcode
    size_t total_size;      // header + bitcode_data size
} bitcode_module_t;

// ============================================================================
// BITCODE LOADER API
// ============================================================================

/**
 * Load bitcode module from memory buffer
 * Returns: 0 on success, -1 on error
 */
int bitcode_load(const void* buffer, size_t size, bitcode_module_t** out);

/**
 * Load bitcode module from FAT16 disk
 * Returns: 0 on success, -1 on error
 */
int bitcode_load_from_disk(void* fs, const char* filename, bitcode_module_t** out);

/**
 * Validate bitcode module header
 * Returns: 0 if valid, -1 if invalid
 */
int bitcode_validate(const bitcode_header_t* header);

/**
 * Free bitcode module memory
 */
void bitcode_free(bitcode_module_t* module);

/**
 * Get bitcode data pointer
 */
static inline const uint8_t* bitcode_get_data(const bitcode_module_t* mod) {
    return mod->bitcode_data;
}

/**
 * Get bitcode size
 */
static inline size_t bitcode_get_size(const bitcode_module_t* mod) {
    return mod->header.bitcode_size;
}

#endif // BITCODE_MODULE_H
