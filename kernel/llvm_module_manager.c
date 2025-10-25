// ============================================================================
// BAREFLOW - LLVM Module Manager Implementation
// ============================================================================

#include "llvm_module_manager.h"
#include "stdlib.h"
#include "vga.h"

extern void serial_puts(const char* str);
extern uint64_t __builtin_ia32_rdtsc(void);

static void print_int(int value) {
    if (value == 0) {
        serial_puts("0");
        return;
    }

    char buf[16];
    int i = 0;
    int is_neg = 0;

    if (value < 0) {
        is_neg = 1;
        value = -value;
    }

    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_neg) buf[i++] = '-';

    for (int j = 0; j < i/2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = tmp;
    }
    buf[i] = '\0';

    serial_puts(buf);
}

static void print_uint64(uint64_t value) {
    if (value == 0) {
        serial_puts("0");
        return;
    }

    char buf[32];
    int i = 0;

    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    for (int j = 0; j < i/2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = tmp;
    }
    buf[i] = '\0';

    serial_puts(buf);
}

void llvm_module_manager_init(llvm_module_manager_t* mgr) {
    if (!mgr) return;

    memset(mgr, 0, sizeof(llvm_module_manager_t));
    serial_puts("[LLVM-MGR] Initialized\n");
}

int llvm_module_register(llvm_module_manager_t* mgr,
                         const char* name,
                         const uint8_t* elf_o0, size_t size_o0,
                         const uint8_t* elf_o1, size_t size_o1,
                         const uint8_t* elf_o2, size_t size_o2,
                         const uint8_t* elf_o3, size_t size_o3) {
    if (!mgr || mgr->module_count >= 16) {
        serial_puts("[LLVM-MGR] ERROR: Manager full or invalid\n");
        return -1;
    }

    llvm_module_t* mod = &mgr->modules[mgr->module_count];
    memset(mod, 0, sizeof(llvm_module_t));

    // Copy name
    int i = 0;
    while (name[i] && i < 31) {
        mod->name[i] = name[i];
        i++;
    }
    mod->name[i] = '\0';

    // Load O0 (required)
    if (elf_o0 && size_o0 > 0) {
        int result = elf_load(elf_o0, size_o0, NULL, &mod->modules[LLVM_OPT_O0]);
        if (result != 0) {
            serial_puts("[LLVM-MGR] ERROR: Failed to load O0 for ");
            serial_puts(name);
            serial_puts("\n");
            return -1;
        }
    } else {
        serial_puts("[LLVM-MGR] ERROR: O0 binary required\n");
        return -1;
    }

    // Load O1 (optional)
    if (elf_o1 && size_o1 > 0) {
        elf_load(elf_o1, size_o1, NULL, &mod->modules[LLVM_OPT_O1]);
    }

    // Load O2 (optional)
    if (elf_o2 && size_o2 > 0) {
        elf_load(elf_o2, size_o2, NULL, &mod->modules[LLVM_OPT_O2]);
    }

    // Load O3 (optional)
    if (elf_o3 && size_o3 > 0) {
        elf_load(elf_o3, size_o3, NULL, &mod->modules[LLVM_OPT_O3]);
    }

    mod->current_level = LLVM_OPT_O0;
    mod->call_count = 0;
    mod->total_cycles = 0;

    int module_id = mgr->module_count;
    mgr->module_count++;
    mgr->total_loaded++;

    serial_puts("[LLVM-MGR] Registered: ");
    serial_puts(name);
    serial_puts(" (ID ");
    print_int(module_id);
    serial_puts(")\n");

    return module_id;
}

int llvm_module_execute(llvm_module_manager_t* mgr, int module_id) {
    if (!mgr || module_id < 0 || module_id >= (int)mgr->module_count) {
        return -1;
    }

    llvm_module_t* mod = &mgr->modules[module_id];
    elf_module_t* elf_mod = mod->modules[mod->current_level];

    if (!elf_mod) {
        serial_puts("[LLVM-MGR] ERROR: No ELF loaded at current level\n");
        return -1;
    }

    // Execute
    int (*func)(void) = (int (*)(void))elf_mod->entry_point;

    uint64_t start = __builtin_ia32_rdtsc();
    int result = func();
    uint64_t end = __builtin_ia32_rdtsc();

    // Update stats
    mod->call_count++;
    mod->total_cycles += (end - start);

    return result;
}

int llvm_module_upgrade(llvm_module_manager_t* mgr, int module_id) {
    if (!mgr || module_id < 0 || module_id >= (int)mgr->module_count) {
        return -1;
    }

    llvm_module_t* mod = &mgr->modules[module_id];

    if (mod->current_level >= LLVM_OPT_O3) {
        serial_puts("[LLVM-MGR] Already at maximum optimization level\n");
        return 0;
    }

    llvm_opt_level_t next_level = mod->current_level + 1;

    if (!mod->modules[next_level]) {
        serial_puts("[LLVM-MGR] Next optimization level not available\n");
        return -1;
    }

    mod->current_level = next_level;

    serial_puts("[LLVM-MGR] Upgraded ");
    serial_puts(mod->name);
    serial_puts(" to O");
    print_int(next_level);
    serial_puts("\n");

    return 0;
}

void llvm_module_print_stats(llvm_module_manager_t* mgr, int module_id) {
    if (!mgr || module_id < 0 || module_id >= (int)mgr->module_count) {
        return;
    }

    llvm_module_t* mod = &mgr->modules[module_id];

    serial_puts("\n=== ");
    serial_puts(mod->name);
    serial_puts(" Statistics ===\n");
    serial_puts("Optimization level: O");
    print_int(mod->current_level);
    serial_puts("\n");
    serial_puts("Call count: ");
    print_int((int)mod->call_count);
    serial_puts("\n");
    serial_puts("Total cycles: ");
    print_uint64(mod->total_cycles);
    serial_puts("\n");

    if (mod->call_count > 0) {
        // Avoid 64-bit division - cast both operands to 32-bit first
        uint32_t total_lo = (uint32_t)mod->total_cycles;
        uint32_t avg = total_lo / mod->call_count;
        serial_puts("Avg cycles/call: ");
        print_int((int)avg);
        serial_puts("\n");
    }
}

int llvm_module_execute_adaptive(llvm_module_manager_t* mgr, int module_id) {
    if (!mgr || module_id < 0 || module_id >= (int)mgr->module_count) {
        return -1;
    }

    llvm_module_t* mod = &mgr->modules[module_id];

    // Check if we should upgrade based on call count
    // Thresholds: 100→O1, 1000→O2, 10000→O3
    if (mod->call_count == 100 && mod->current_level == LLVM_OPT_O0) {
        llvm_module_upgrade(mgr, module_id);
    } else if (mod->call_count == 1000 && mod->current_level == LLVM_OPT_O1) {
        llvm_module_upgrade(mgr, module_id);
    } else if (mod->call_count == 10000 && mod->current_level == LLVM_OPT_O2) {
        llvm_module_upgrade(mgr, module_id);
    }

    // Execute
    return llvm_module_execute(mgr, module_id);
}
