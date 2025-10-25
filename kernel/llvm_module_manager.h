// ============================================================================
// BAREFLOW - LLVM Module Manager
// ============================================================================
// Manages loading of LLVM-compiled modules at different optimization levels
// Supports adaptive optimization via multi-level ELF loading

#ifndef LLVM_MODULE_MANAGER_H
#define LLVM_MODULE_MANAGER_H

#include <stdint.h>
#include "elf_loader.h"

// Optimization levels
typedef enum {
    LLVM_OPT_O0 = 0,  // No optimization
    LLVM_OPT_O1 = 1,  // Basic optimization
    LLVM_OPT_O2 = 2,  // Aggressive optimization
    LLVM_OPT_O3 = 3,  // Maximum optimization
    LLVM_OPT_COUNT = 4
} llvm_opt_level_t;

// LLVM module with multiple optimization levels
typedef struct {
    char name[32];                          // Module name
    elf_module_t* modules[LLVM_OPT_COUNT];  // ELF modules for each opt level
    llvm_opt_level_t current_level;         // Currently active level
    uint32_t call_count;                    // Number of calls
    uint64_t total_cycles;                  // Total execution cycles
} llvm_module_t;

// LLVM module manager
typedef struct {
    llvm_module_t modules[16];  // Max 16 LLVM modules
    uint32_t module_count;
    uint32_t total_loaded;
} llvm_module_manager_t;

/**
 * Initialize LLVM module manager
 */
void llvm_module_manager_init(llvm_module_manager_t* mgr);

/**
 * Register a new LLVM module with embedded ELF binaries
 *
 * @param mgr Module manager
 * @param name Module name
 * @param elf_o0 Embedded O0 ELF binary
 * @param size_o0 Size of O0 binary
 * @param elf_o1 Embedded O1 ELF binary (or NULL)
 * @param size_o1 Size of O1 binary
 * @param elf_o2 Embedded O2 ELF binary (or NULL)
 * @param size_o2 Size of O2 binary
 * @param elf_o3 Embedded O3 ELF binary (or NULL)
 * @param size_o3 Size of O3 binary
 * @return Module ID or -1 on error
 */
int llvm_module_register(llvm_module_manager_t* mgr,
                         const char* name,
                         const uint8_t* elf_o0, size_t size_o0,
                         const uint8_t* elf_o1, size_t size_o1,
                         const uint8_t* elf_o2, size_t size_o2,
                         const uint8_t* elf_o3, size_t size_o3);

/**
 * Execute module at current optimization level
 *
 * @param mgr Module manager
 * @param module_id Module ID
 * @return Execution result
 */
int llvm_module_execute(llvm_module_manager_t* mgr, int module_id);

/**
 * Upgrade module to next optimization level
 *
 * @param mgr Module manager
 * @param module_id Module ID
 * @return 0 on success, -1 on error
 */
int llvm_module_upgrade(llvm_module_manager_t* mgr, int module_id);

/**
 * Get module statistics
 */
void llvm_module_print_stats(llvm_module_manager_t* mgr, int module_id);

/**
 * Adaptive execution with automatic optimization
 * Upgrades optimization level based on call count thresholds:
 * - 0-99 calls: O0
 * - 100-999 calls: O1
 * - 1000-9999 calls: O2
 * - 10000+ calls: O3
 */
int llvm_module_execute_adaptive(llvm_module_manager_t* mgr, int module_id);

#endif // LLVM_MODULE_MANAGER_H
