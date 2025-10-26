/**
 * Profiling Data Export System
 *
 * Exports module profiling statistics in JSON format via serial port (COM1)
 * for offline analysis and LLVM profile-guided optimization.
 */

#ifndef PROFILING_EXPORT_H
#define PROFILING_EXPORT_H

#include <stdint.h>
#include "module_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SERIAL PORT INTERFACE
// ============================================================================

/**
 * Initialize serial port (COM1) for profiling export
 * Baud rate: 115200, 8N1 (8 bits, no parity, 1 stop bit)
 *
 * Returns: 0 on success, -1 on failure
 */
int serial_init(void);

/**
 * Write a single character to serial port
 */
void serial_putchar(char c);

/**
 * Write a null-terminated string to serial port
 */
void serial_puts(const char* str);

/**
 * Write formatted integer to serial port (decimal)
 */
void serial_put_int(int value);

/**
 * Write formatted unsigned integer to serial port (decimal)
 */
void serial_put_uint(unsigned int value);

/**
 * Write formatted 64-bit unsigned integer to serial port (decimal)
 */
void serial_put_uint64(uint64_t value);

// ============================================================================
// PROFILING EXPORT
// ============================================================================

/**
 * Export all module profiling data in JSON format via serial port
 *
 * Format:
 * {
 *   "format_version": "1.0",
 *   "timestamp_cycles": <rdtsc value>,
 *   "modules": [
 *     {
 *       "name": "module_name",
 *       "calls": <call_count>,
 *       "total_cycles": <total_cycles>,
 *       "min_cycles": <min>,
 *       "max_cycles": <max>,
 *       "avg_cycles": <avg>,
 *       "code_address": "0xXXXXXXXX",
 *       "code_size": <bytes>
 *     },
 *     ...
 *   ]
 * }
 *
 * Parameters:
 *   mgr - Module manager containing profiling data
 *
 * Returns: 0 on success, -1 on failure
 */
int profiling_export_json(const module_manager_t* mgr);

/**
 * Export profiling data and trigger host-side recompilation
 *
 * This is the main entry point for the profile-guided optimization workflow:
 * 1. Exports profiling data via serial port
 * 2. Host receives data and identifies hot functions
 * 3. Host recompiles modules with LLVM -O2/-O3 + PGO
 * 4. Host updates module cache on disk
 * 5. Next boot loads optimized versions
 *
 * Parameters:
 *   mgr - Module manager
 *
 * Returns: 0 on success, -1 on failure
 */
int profiling_trigger_export(const module_manager_t* mgr);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Get current timestamp from RDTSC for profiling export
 */
uint64_t profiling_get_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif // PROFILING_EXPORT_H
