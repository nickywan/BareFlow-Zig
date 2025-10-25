// ============================================================================
// BAREFLOW - Bitcode Module Loader
// ============================================================================

#include "bitcode_module.h"
#include "stdlib.h"
#include "vga.h"
#include "fat16.h"

// Validate bitcode header
int bitcode_validate(const bitcode_header_t* header) {
    if (!header) return -1;

    // Check magic
    if (header->magic != BITCODE_MAGIC) {
        return -1;
    }

    // Check size is reasonable (< 1MB)
    if (header->bitcode_size == 0 || header->bitcode_size > 1024 * 1024) {
        return -1;
    }

    // Check opt level
    if (header->opt_level > 3) {
        return -1;
    }

    return 0;
}

// Load bitcode from memory buffer
int bitcode_load(const void* buffer, size_t size, bitcode_module_t** out) {
    if (!buffer || size < sizeof(bitcode_header_t) || !out) {
        return -1;
    }

    const bitcode_header_t* header = (const bitcode_header_t*)buffer;

    // Validate header
    if (bitcode_validate(header) != 0) {
        return -1;
    }

    // Check buffer size
    size_t expected_size = sizeof(bitcode_header_t) + header->bitcode_size;
    if (size < expected_size) {
        return -1;
    }

    // Allocate module structure
    bitcode_module_t* module = (bitcode_module_t*)malloc(sizeof(bitcode_module_t));
    if (!module) {
        return -1;
    }

    // Copy header
    memcpy(&module->header, header, sizeof(bitcode_header_t));

    // Allocate and copy bitcode data
    module->bitcode_data = (uint8_t*)malloc(header->bitcode_size);
    if (!module->bitcode_data) {
        free(module);
        return -1;
    }

    const uint8_t* bitcode_src = (const uint8_t*)buffer + sizeof(bitcode_header_t);
    memcpy(module->bitcode_data, bitcode_src, header->bitcode_size);

    module->total_size = expected_size;

    *out = module;
    return 0;
}

// Free bitcode module
void bitcode_free(bitcode_module_t* module) {
    if (!module) return;

    if (module->bitcode_data) {
        free(module->bitcode_data);
    }
    free(module);
}

// Load bitcode from FAT16 disk
int bitcode_load_from_disk(void* fs_ptr, const char* filename, bitcode_module_t** out) {
    if (!fs_ptr || !filename || !out) {
        return -1;
    }

    fat16_fs_t* fs = (fat16_fs_t*)fs_ptr;
    fat16_file_t file;

    // Open the bitcode file
    if (fat16_open(fs, filename, &file) != 0) {
        terminal_writestring("Failed to open bitcode file: ");
        terminal_writestring(filename);
        terminal_writestring("\n");
        return -1;
    }

    // Get file size
    uint32_t file_size = fat16_get_file_size(&file);
    if (file_size < sizeof(bitcode_header_t)) {
        terminal_writestring("Bitcode file too small\n");
        fat16_close(&file);
        return -1;
    }

    // Allocate buffer for entire file
    uint8_t* buffer = (uint8_t*)malloc(file_size);
    if (!buffer) {
        terminal_writestring("Failed to allocate buffer for bitcode\n");
        fat16_close(&file);
        return -1;
    }

    // Read entire file
    int bytes_read = fat16_read(fs, &file, buffer, file_size);
    fat16_close(&file);

    if (bytes_read != (int)file_size) {
        terminal_writestring("Failed to read bitcode file\n");
        free(buffer);
        return -1;
    }

    // Load from memory buffer
    int result = bitcode_load(buffer, file_size, out);

    // Free temporary buffer (bitcode_load makes its own copy)
    free(buffer);

    return result;
}
