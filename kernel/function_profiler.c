// ============================================================================
// BAREFLOW - Function-Level Profiling Implementation
// ============================================================================

#include "function_profiler.h"
#include <stddef.h>

// External functions from stdlib
extern void* memset(void* s, int c, size_t n);
extern size_t strlen(const char* s);
extern int strcmp(const char* s1, const char* s2);
extern void* malloc(size_t size);
extern void free(void* ptr);

// Helper: string copy
static char* string_copy(const char* src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char* dst = (char*)malloc(len + 1);
    if (!dst) return NULL;
    for (size_t i = 0; i <= len; i++) {
        dst[i] = src[i];
    }
    return dst;
}

// ============================================================================
// Initialization
// ============================================================================

void function_profiler_init(function_profiler_t* profiler, bool enable_jit) {
    if (!profiler) return;

    memset(profiler, 0, sizeof(function_profiler_t));
    profiler->jit_enabled = enable_jit;
}

// ============================================================================
// Function Registration
// ============================================================================

int function_profiler_register(
    function_profiler_t* profiler,
    const char* func_name,
    const char* module_name,
    void* address)
{
    if (!profiler || !func_name || !module_name) return -1;
    if (profiler->function_count >= MAX_FUNCTIONS) return -1;

    int id = profiler->function_count;
    function_profile_t* func = &profiler->functions[id];

    func->name = func_name;  // Assume string is static (from embedded modules)
    func->module_name = module_name;
    func->address = address;
    func->call_count = 0;
    func->total_cycles = 0;
    func->min_cycles = UINT64_MAX;
    func->max_cycles = 0;
    func->opt_level = OPT_LEVEL_O0;
    func->needs_recompile = false;
    func->is_hot = false;

    profiler->function_count++;
    return id;
}

// ============================================================================
// Call Recording
// ============================================================================

void function_profiler_record(
    function_profiler_t* profiler,
    int func_id,
    uint64_t cycles)
{
    if (!profiler || func_id < 0 || func_id >= profiler->function_count) return;

    function_profile_t* func = &profiler->functions[func_id];

    func->call_count++;
    func->total_cycles += cycles;

    if (cycles < func->min_cycles) {
        func->min_cycles = cycles;
    }
    if (cycles > func->max_cycles) {
        func->max_cycles = cycles;
    }

    profiler->total_calls++;

    // Check if JIT recompilation is needed
    if (profiler->jit_enabled && !func->needs_recompile) {
        uint64_t calls = func->call_count;
        opt_level_t current = func->opt_level;

        // Trigger recompilation at thresholds
        if (current == OPT_LEVEL_O0 && calls >= JIT_THRESHOLD_O1) {
            func->needs_recompile = true;
        }
        else if (current == OPT_LEVEL_O1 && calls >= JIT_THRESHOLD_O2) {
            func->needs_recompile = true;
        }
        else if (current == OPT_LEVEL_O2 && calls >= JIT_THRESHOLD_O3) {
            func->needs_recompile = true;
        }
    }
}

// ============================================================================
// JIT Recompilation Checks
// ============================================================================

bool function_profiler_needs_recompile(
    function_profiler_t* profiler,
    int func_id)
{
    if (!profiler || func_id < 0 || func_id >= profiler->function_count) {
        return false;
    }

    return profiler->functions[func_id].needs_recompile;
}

void function_profiler_mark_recompiled(
    function_profiler_t* profiler,
    int func_id,
    opt_level_t new_level)
{
    if (!profiler || func_id < 0 || func_id >= profiler->function_count) return;

    function_profile_t* func = &profiler->functions[func_id];
    func->opt_level = new_level;
    func->needs_recompile = false;
}

// ============================================================================
// Hot Function Detection
// ============================================================================

int function_profiler_get_hot_functions(
    function_profiler_t* profiler,
    int* hot_func_ids,
    int max_count)
{
    if (!profiler || !hot_func_ids || max_count <= 0) return 0;

    // Simple bubble sort by total_cycles (descending)
    // Create array of indices
    int indices[MAX_FUNCTIONS];
    for (int i = 0; i < profiler->function_count; i++) {
        indices[i] = i;
    }

    // Sort indices by total_cycles (bubble sort - simple for small N)
    for (int i = 0; i < profiler->function_count - 1; i++) {
        for (int j = 0; j < profiler->function_count - i - 1; j++) {
            uint64_t cycles_j = profiler->functions[indices[j]].total_cycles;
            uint64_t cycles_j1 = profiler->functions[indices[j + 1]].total_cycles;

            if (cycles_j < cycles_j1) {
                // Swap
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }

    // Copy top max_count to output
    int count = (max_count < profiler->function_count) ? max_count : profiler->function_count;
    for (int i = 0; i < count; i++) {
        hot_func_ids[i] = indices[i];
        profiler->functions[indices[i]].is_hot = true;
    }

    return count;
}

// ============================================================================
// Statistics Printing (for VGA debugging)
// ============================================================================

// External VGA functions
extern void terminal_writestring(const char* str);
extern void terminal_putchar(char c);

static void print_int(int num) {
    if (num == 0) {
        terminal_putchar('0');
        return;
    }

    if (num < 0) {
        terminal_putchar('-');
        num = -num;
    }

    char buffer[12];
    int i = 0;

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        terminal_putchar(buffer[j]);
    }
}

// Simplified: Print only lower 32 bits to avoid __udivdi3
static void print_uint64(uint64_t num) {
    // For bare-metal, just print lower 32 bits
    uint32_t lower = (uint32_t)num;

    if (lower == 0) {
        terminal_putchar('0');
        return;
    }

    char buffer[16];
    int i = 0;

    while (lower > 0) {
        buffer[i++] = '0' + (lower % 10);
        lower /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        terminal_putchar(buffer[j]);
    }

    // Indicate if upper bits are non-zero
    if ((num >> 32) != 0) {
        terminal_putchar('+');
    }
}

void function_profiler_print_stats(function_profiler_t* profiler) {
    if (!profiler) return;

    terminal_writestring("\n=== FUNCTION PROFILER STATISTICS ===\n");
    terminal_writestring("Total function calls: ");
    print_uint64(profiler->total_calls);
    terminal_writestring("\n");
    terminal_writestring("Functions tracked: ");
    print_int(profiler->function_count);
    terminal_writestring("\n\n");

    terminal_writestring("Function Details:\n");
    for (int i = 0; i < profiler->function_count; i++) {
        function_profile_t* func = &profiler->functions[i];

        terminal_writestring("  ");
        terminal_writestring(func->module_name);
        terminal_writestring("::");
        terminal_writestring(func->name);
        terminal_writestring("\n");

        terminal_writestring("    Calls: ");
        print_uint64(func->call_count);
        terminal_writestring(", Total cycles: ");
        print_uint64(func->total_cycles);
        terminal_writestring("\n");

        if (func->call_count > 0) {
            // Avoid 64-bit division - approximate with 32-bit
            uint32_t avg_approx = (uint32_t)func->total_cycles / (uint32_t)func->call_count;
            terminal_writestring("    Avg: ");
            print_int(avg_approx);
            terminal_writestring(", Min: ");
            print_uint64(func->min_cycles);
            terminal_writestring(", Max: ");
            print_uint64(func->max_cycles);
            terminal_writestring("\n");
        }

        terminal_writestring("    Opt level: O");
        print_int(func->opt_level);
        if (func->needs_recompile) {
            terminal_writestring(" [NEEDS RECOMPILE]");
        }
        if (func->is_hot) {
            terminal_writestring(" [HOT]");
        }
        terminal_writestring("\n\n");
    }
}

// ============================================================================
// JSON Export (for automation tools)
// ============================================================================

char* function_profiler_export_json(function_profiler_t* profiler) {
    if (!profiler) return NULL;

    // TODO: Implement JSON export with proper sprintf/vsprintf
    // For now, use VGA printing via function_profiler_print_stats()
    // JSON export will be added when we integrate llvm-libc's sprintf

    return NULL;
}
