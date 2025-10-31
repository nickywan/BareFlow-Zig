#include "profiler.h"
#include <stdint.h>

// Bare-metal definitions
#define NULL ((void*)0)
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL

// Forward declaration - serial.c
extern void serial_puts(const char* str);

// Global profiler instance
static Profiler g_profiler = {0};

// Read CPU cycle counter (rdtsc)
static inline uint64_t read_tsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// Simple integer to string conversion
static void uint64_to_str(uint64_t value, char* buf, int buf_size) {
    if (buf_size < 2) return;

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    // Build string in reverse
    int pos = 0;
    while (value > 0 && pos < buf_size - 1) {
        buf[pos++] = '0' + (value % 10);
        value /= 10;
    }
    buf[pos] = '\0';

    // Reverse string
    for (int i = 0; i < pos / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[pos - 1 - i];
        buf[pos - 1 - i] = tmp;
    }
}

// Initialize profiler
void profiler_init(void) {
    for (int i = 0; i < MAX_PROFILED_FUNCTIONS; i++) {
        g_profiler.entries[i].name = NULL;
        g_profiler.entries[i].call_count = 0;
        g_profiler.entries[i].total_cycles = 0;
        g_profiler.entries[i].min_cycles = UINT64_MAX;
        g_profiler.entries[i].max_cycles = 0;
    }
    g_profiler.num_entries = 0;
    g_profiler.enabled = 1;  // Enabled by default

    serial_puts("[Profiler] Initialized (rdtsc-based)\n");
}

// Register a new function for profiling
int profiler_register(const char* name) {
    if (g_profiler.num_entries >= MAX_PROFILED_FUNCTIONS) {
        serial_puts("[Profiler] ERROR: Max functions reached\n");
        return -1;
    }

    int index = g_profiler.num_entries++;
    g_profiler.entries[index].name = name;
    g_profiler.entries[index].call_count = 0;
    g_profiler.entries[index].total_cycles = 0;
    g_profiler.entries[index].min_cycles = UINT64_MAX;
    g_profiler.entries[index].max_cycles = 0;

    return index;
}

// Start timing a function call
uint64_t profiler_start(void) {
    if (!g_profiler.enabled) return 0;
    return read_tsc();
}

// End timing and record results
void profiler_end(int func_index, uint64_t start_cycles) {
    if (!g_profiler.enabled || func_index < 0 || func_index >= g_profiler.num_entries) {
        return;
    }

    uint64_t end_cycles = read_tsc();
    uint64_t elapsed = end_cycles - start_cycles;

    ProfileEntry* entry = &g_profiler.entries[func_index];
    entry->call_count++;
    entry->total_cycles += elapsed;

    if (elapsed < entry->min_cycles) {
        entry->min_cycles = elapsed;
    }
    if (elapsed > entry->max_cycles) {
        entry->max_cycles = elapsed;
    }
}

// Print profiling report
void profiler_report(void) {
    serial_puts("\n");
    serial_puts("========================================\n");
    serial_puts("  Profiler Report (\"Grow to Shrink\")\n");
    serial_puts("========================================\n\n");

    if (g_profiler.num_entries == 0) {
        serial_puts("No functions profiled.\n\n");
        return;
    }

    char buf[32];

    for (int i = 0; i < g_profiler.num_entries; i++) {
        ProfileEntry* entry = &g_profiler.entries[i];

        if (entry->call_count == 0) continue;

        // Function name
        serial_puts("[");
        serial_puts(entry->name);
        serial_puts("]\n");

        // Call count
        serial_puts("  Calls:       ");
        uint64_to_str(entry->call_count, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts("\n");

        // Total cycles
        serial_puts("  Total:       ");
        uint64_to_str(entry->total_cycles, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts(" cycles\n");

        // Average cycles
        uint64_t avg = entry->total_cycles / entry->call_count;
        serial_puts("  Avg:         ");
        uint64_to_str(avg, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts(" cycles\n");

        // Min cycles
        serial_puts("  Min:         ");
        uint64_to_str(entry->min_cycles, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts(" cycles\n");

        // Max cycles
        serial_puts("  Max:         ");
        uint64_to_str(entry->max_cycles, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts(" cycles\n\n");
    }

    serial_puts("========================================\n");
    serial_puts("  Hot Paths (candidates for JIT -O3)\n");
    serial_puts("========================================\n\n");

    // Find top 5 hot paths by total cycles
    int hot_indices[5] = {-1, -1, -1, -1, -1};
    profiler_get_hot_paths(hot_indices, 5);

    for (int i = 0; i < 5 && hot_indices[i] != -1; i++) {
        ProfileEntry* entry = &g_profiler.entries[hot_indices[i]];

        serial_puts("  ");
        uint64_to_str(i + 1, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts(". ");
        serial_puts(entry->name);
        serial_puts(" (");
        uint64_to_str(entry->total_cycles, buf, sizeof(buf));
        serial_puts(buf);
        serial_puts(" cycles)\n");
    }

    serial_puts("\n");
    serial_puts("Next: Boot 10-100 → JIT compile hot paths\n");
    serial_puts("      Boot 100+   → Dead code elimination\n");
    serial_puts("========================================\n\n");
}

// Enable profiler
void profiler_enable(void) {
    g_profiler.enabled = 1;
    serial_puts("[Profiler] Enabled\n");
}

// Disable profiler
void profiler_disable(void) {
    g_profiler.enabled = 0;
    serial_puts("[Profiler] Disabled\n");
}

// Identify hot paths (sorted by total cycles, descending)
void profiler_get_hot_paths(int* hot_indices, int max_count) {
    // Simple selection sort to find top N
    for (int i = 0; i < max_count; i++) {
        hot_indices[i] = -1;
        uint64_t max_cycles = 0;

        for (int j = 0; j < g_profiler.num_entries; j++) {
            // Skip if already selected
            int already_selected = 0;
            for (int k = 0; k < i; k++) {
                if (hot_indices[k] == j) {
                    already_selected = 1;
                    break;
                }
            }
            if (already_selected) continue;

            // Find entry with most total cycles
            if (g_profiler.entries[j].total_cycles > max_cycles) {
                max_cycles = g_profiler.entries[j].total_cycles;
                hot_indices[i] = j;
            }
        }
    }
}
