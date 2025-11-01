/**
 * JIT Profiling System - Implementation
 */

#include "profile.h"
#include "../cpu/features.h"
#include "../memory/string.h"
#include "../io/serial.h"

void jit_profile_init(jit_profile_t* prof) {
    prof->num_functions = 0;
    for (int i = 0; i < JIT_MAX_FUNCTIONS; i++) {
        prof->functions[i].name[0] = '\0';
        prof->functions[i].call_count = 0;
        prof->functions[i].total_cycles = 0;
        prof->functions[i].min_cycles = UINT64_MAX;
        prof->functions[i].max_cycles = 0;
        prof->functions[i].last_start = 0;
        prof->functions[i].active = 0;
    }
}

// Find or create function entry
static jit_profile_entry_t* find_or_create_entry(jit_profile_t* prof, const char* func_name) {
    // Search for existing entry
    for (int i = 0; i < prof->num_functions; i++) {
        if (strcmp(prof->functions[i].name, func_name) == 0) {
            return &prof->functions[i];
        }
    }

    // Create new entry
    if (prof->num_functions >= JIT_MAX_FUNCTIONS) {
        return NULL;  // Out of space
    }

    jit_profile_entry_t* entry = &prof->functions[prof->num_functions];
    strncpy(entry->name, func_name, JIT_MAX_FUNC_NAME - 1);
    entry->name[JIT_MAX_FUNC_NAME - 1] = '\0';
    entry->call_count = 0;
    entry->total_cycles = 0;
    entry->min_cycles = UINT64_MAX;
    entry->max_cycles = 0;
    entry->active = 0;
    prof->num_functions++;

    return entry;
}

void jit_profile_begin(jit_profile_t* prof, const char* func_name) {
    jit_profile_entry_t* entry = find_or_create_entry(prof, func_name);
    if (!entry || entry->active) {
        return;  // Nested calls not supported
    }

    entry->last_start = cpu_rdtsc();
    entry->active = 1;
}

void jit_profile_end(jit_profile_t* prof, const char* func_name) {
    uint64_t end_time = cpu_rdtsc();

    jit_profile_entry_t* entry = find_or_create_entry(prof, func_name);
    if (!entry || !entry->active) {
        return;
    }

    uint64_t cycles = end_time - entry->last_start;

    entry->call_count++;
    entry->total_cycles += cycles;

    if (cycles < entry->min_cycles) {
        entry->min_cycles = cycles;
    }
    if (cycles > entry->max_cycles) {
        entry->max_cycles = cycles;
    }

    entry->active = 0;
}

uint64_t jit_get_call_count(jit_profile_t* prof, const char* func_name) {
    for (int i = 0; i < prof->num_functions; i++) {
        if (strcmp(prof->functions[i].name, func_name) == 0) {
            return prof->functions[i].call_count;
        }
    }
    return 0;
}

uint64_t jit_get_avg_cycles(jit_profile_t* prof, const char* func_name) {
    for (int i = 0; i < prof->num_functions; i++) {
        if (strcmp(prof->functions[i].name, func_name) == 0) {
            if (prof->functions[i].call_count == 0) {
                return 0;
            }
            return prof->functions[i].total_cycles / prof->functions[i].call_count;
        }
    }
    return 0;
}

void jit_print_stats(jit_profile_t* prof, const char* func_name) {
    for (int i = 0; i < prof->num_functions; i++) {
        if (strcmp(prof->functions[i].name, func_name) == 0) {
            jit_profile_entry_t* e = &prof->functions[i];

            serial_puts(func_name);
            serial_puts(": calls=");
            serial_put_uint64(e->call_count);
            serial_puts(", avg=");
            serial_put_uint64(e->call_count > 0 ? e->total_cycles / e->call_count : 0);
            serial_puts(", min=");
            serial_put_uint64(e->min_cycles != UINT64_MAX ? e->min_cycles : 0);
            serial_puts(", max=");
            serial_put_uint64(e->max_cycles);
            serial_puts("\n");
            return;
        }
    }

    serial_puts(func_name);
    serial_puts(": not found\n");
}

void jit_print_all_stats(jit_profile_t* prof) {
    serial_puts("\n=== JIT Profiling Statistics ===\n");
    for (int i = 0; i < prof->num_functions; i++) {
        jit_print_stats(prof, prof->functions[i].name);
    }
    serial_puts("================================\n\n");
}
