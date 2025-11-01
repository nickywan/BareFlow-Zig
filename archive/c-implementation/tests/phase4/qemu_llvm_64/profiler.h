#ifndef PROFILER_H
#define PROFILER_H

#include <stdint.h>

// Maximum number of profiled functions
#define MAX_PROFILED_FUNCTIONS 32

// Profiling entry for a single function
typedef struct {
    const char* name;           // Function name
    uint64_t call_count;        // Number of calls
    uint64_t total_cycles;      // Total CPU cycles (rdtsc)
    uint64_t min_cycles;        // Minimum cycles per call
    uint64_t max_cycles;        // Maximum cycles per call
} ProfileEntry;

// Profiler state
typedef struct {
    ProfileEntry entries[MAX_PROFILED_FUNCTIONS];
    int num_entries;
    int enabled;                // 0 = disabled, 1 = enabled
} Profiler;

// Initialize profiler
void profiler_init(void);

// Register a new function for profiling (returns index)
int profiler_register(const char* name);

// Start timing a function call
uint64_t profiler_start(void);

// End timing and record results
void profiler_end(int func_index, uint64_t start_cycles);

// Print profiling report
void profiler_report(void);

// Enable/disable profiler
void profiler_enable(void);
void profiler_disable(void);

// Identify hot paths (returns array of function indices sorted by total cycles)
void profiler_get_hot_paths(int* hot_indices, int max_count);

#endif // PROFILER_H
