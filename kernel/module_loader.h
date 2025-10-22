// ============================================================================
// BAREFLOW - Module Loader (Bare-Metal)
// ============================================================================
// File: kernel/module_loader.h
// Purpose: Load and execute pre-compiled native code modules with profiling
// Approach: AOT compilation + dynamic loading (realistic for bare-metal)
// ============================================================================

#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include <stddef.h>
#include <stdint.h>

// ============================================================================
// MODULE SYSTEM
// ============================================================================

#define MAX_MODULES 16
#define MAX_MODULE_NAME 32
#define MODULE_MAGIC 0x4D4F4442  // "MODB" (MODule Bare-metal)

// Module metadata (embedded in compiled module)
typedef struct {
    uint32_t magic;              // Must be MODULE_MAGIC
    char name[MAX_MODULE_NAME];  // Module name
    void* entry_point;           // Function entry point
    uint32_t code_size;          // Size of code section
    uint32_t version;            // Module version
} __attribute__((packed)) module_header_t;

// Module profiling data
typedef struct {
    char name[MAX_MODULE_NAME];
    void* code_ptr;
    uint64_t call_count;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint32_t code_size;
    int loaded;
} module_profile_t;

// Module manager
typedef struct {
    module_profile_t modules[MAX_MODULES];
    uint32_t num_modules;
    uint64_t total_calls;
} module_manager_t;

// ============================================================================
// CORE API
// ============================================================================

// Initialize module system
void module_init(module_manager_t* mgr);

// Load a module from memory
int module_load(module_manager_t* mgr, const void* module_data, size_t size);

// Find a module by name
module_profile_t* module_find(module_manager_t* mgr, const char* name);

// Execute a module with profiling
typedef int (*module_func_t)(void);
int module_execute(module_manager_t* mgr, const char* name);

// Get module statistics
void module_print_stats(module_manager_t* mgr, const char* name);
void module_print_all_stats(module_manager_t* mgr);

// ============================================================================
// PROFILING HELPERS
// ============================================================================

// Read CPU timestamp counter (for cycle counting)
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

#endif // MODULE_LOADER_H
