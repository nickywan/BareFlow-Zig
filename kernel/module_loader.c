// ============================================================================
// BAREFLOW - Module Loader Implementation
// ============================================================================

#include "module_loader.h"
#include "vga.h"

// Forward declarations from stdlib
extern void* memcpy(void* dest, const void* src, size_t n);
extern void* memset(void* s, int c, size_t n);
extern size_t strlen(const char* s);
extern char* strcpy(char* dest, const char* src);

// Helper to print integers
static void print_int(int num);
static void print_u64(uint64_t num);
static void print_hex(uint32_t num);

// Simple 64-bit division (avoids __udivdi3 dependency)
static uint64_t udiv64(uint64_t dividend, uint64_t divisor) {
    if (divisor == 0) return 0;

    uint64_t quotient = 0;
    uint64_t remainder = 0;

    for (int i = 63; i >= 0; i--) {
        remainder <<= 1;
        remainder |= (dividend >> i) & 1;

        if (remainder >= divisor) {
            remainder -= divisor;
            quotient |= (1ULL << i);
        }
    }

    return quotient;
}

// ============================================================================
// MODULE MANAGER
// ============================================================================

void module_init(module_manager_t* mgr) {
    memset(mgr, 0, sizeof(module_manager_t));
    mgr->num_modules = 0;
    mgr->total_calls = 0;
}

int module_load(module_manager_t* mgr, const void* module_data, size_t size) {
    if (!mgr || !module_data || mgr->num_modules >= MAX_MODULES) {
        return -1;
    }

    (void)size;  // Size validation could be added later

    // Validate module header
    const module_header_t* header = (const module_header_t*)module_data;

    if (header->magic != MODULE_MAGIC) {
        terminal_writestring("[ERROR] Invalid module magic\n");
        return -2;
    }

    // Check if module already loaded (simple string comparison)
    for (uint32_t i = 0; i < mgr->num_modules; i++) {
        if (mgr->modules[i].loaded) {
            // Compare names
            const char* a = mgr->modules[i].name;
            const char* b = header->name;
            int match = 1;

            while (*a && *b) {
                if (*a != *b) {
                    match = 0;
                    break;
                }
                a++;
                b++;
            }

            if (match && *a == *b) {
                terminal_writestring("[WARN] Module already loaded: ");
                terminal_writestring(header->name);
                terminal_writestring("\n");
                return -3;
            }
        }
    }

    // Add module to manager
    module_profile_t* prof = &mgr->modules[mgr->num_modules];
    memset(prof, 0, sizeof(module_profile_t));

    // Copy name safely
    size_t name_len = strlen(header->name);
    if (name_len >= MAX_MODULE_NAME) name_len = MAX_MODULE_NAME - 1;
    memcpy(prof->name, header->name, name_len);
    prof->name[name_len] = '\0';

    prof->code_ptr = header->entry_point;
    prof->code_size = header->code_size;
    prof->call_count = 0;
    prof->total_cycles = 0;
    prof->min_cycles = UINT64_MAX;
    prof->max_cycles = 0;
    prof->loaded = 1;

    mgr->num_modules++;

    terminal_writestring("[OK] Module loaded: ");
    terminal_writestring(prof->name);
    terminal_writestring(" (entry: 0x");
    print_hex((uint32_t)prof->code_ptr);
    terminal_writestring(")\n");

    return 0;
}

module_profile_t* module_find(module_manager_t* mgr, const char* name) {
    if (!mgr || !name) return NULL;

    for (uint32_t i = 0; i < mgr->num_modules; i++) {
        if (mgr->modules[i].loaded) {
            // Simple string comparison (since we don't have strcmp)
            const char* a = mgr->modules[i].name;
            const char* b = name;
            int match = 1;

            while (*a && *b) {
                if (*a != *b) {
                    match = 0;
                    break;
                }
                a++;
                b++;
            }

            if (match && *a == *b) {
                return &mgr->modules[i];
            }
        }
    }

    return NULL;
}

int module_execute(module_manager_t* mgr, const char* name) {
    module_profile_t* mod = module_find(mgr, name);
    if (!mod) {
        terminal_writestring("[ERROR] Module not found: ");
        terminal_writestring(name);
        terminal_writestring("\n");
        return -1;
    }

    // Profile execution
    uint64_t start = rdtsc();

    // Execute the module
    module_func_t func = (module_func_t)mod->code_ptr;
    int result = func();

    uint64_t end = rdtsc();
    uint64_t cycles = end - start;

    // Update profiling stats
    mod->call_count++;
    mod->total_cycles += cycles;

    if (cycles < mod->min_cycles) mod->min_cycles = cycles;
    if (cycles > mod->max_cycles) mod->max_cycles = cycles;

    mgr->total_calls++;

    return result;
}

void module_print_stats(module_manager_t* mgr, const char* name) {
    module_profile_t* mod = module_find(mgr, name);
    if (!mod) {
        terminal_writestring("[ERROR] Module not found\n");
        return;
    }

    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\n=== Module Stats: ");
    terminal_writestring(mod->name);
    terminal_writestring(" ===\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("  Code address:  0x");
    print_hex((uint32_t)mod->code_ptr);
    terminal_writestring("\n");

    terminal_writestring("  Code size:     ");
    print_int(mod->code_size);
    terminal_writestring(" bytes\n");

    terminal_writestring("  Calls:         ");
    print_u64(mod->call_count);
    terminal_writestring("\n");

    terminal_writestring("  Total cycles:  ");
    print_u64(mod->total_cycles);
    terminal_writestring("\n");

    if (mod->call_count > 0) {
        terminal_writestring("  Avg cycles:    ");
        print_u64(udiv64(mod->total_cycles, mod->call_count));
        terminal_writestring("\n");

        terminal_writestring("  Min cycles:    ");
        print_u64(mod->min_cycles);
        terminal_writestring("\n");

        terminal_writestring("  Max cycles:    ");
        print_u64(mod->max_cycles);
        terminal_writestring("\n");
    }
}

void module_print_all_stats(module_manager_t* mgr) {
    terminal_setcolor(VGA_CYAN, VGA_BLACK);
    terminal_writestring("\n========================================\n");
    terminal_writestring("      MODULE SYSTEM STATISTICS\n");
    terminal_writestring("========================================\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    terminal_writestring("Total modules loaded: ");
    print_int(mgr->num_modules);
    terminal_writestring("\n");

    terminal_writestring("Total calls:          ");
    print_u64(mgr->total_calls);
    terminal_writestring("\n\n");

    for (uint32_t i = 0; i < mgr->num_modules; i++) {
        if (mgr->modules[i].loaded) {
            module_print_stats(mgr, mgr->modules[i].name);
        }
    }

    terminal_setcolor(VGA_CYAN, VGA_BLACK);
    terminal_writestring("========================================\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

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

static void print_u64(uint64_t num) {
    if (num == 0) {
        terminal_putchar('0');
        return;
    }

    char buffer[24];
    int i = 0;

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        terminal_putchar(buffer[j]);
    }
}

static void print_hex(uint32_t num) {
    char hex[] = "0123456789abcdef";
    char buffer[9];

    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex[num & 0xF];
        num >>= 4;
    }
    buffer[8] = '\0';

    terminal_writestring(buffer);
}
