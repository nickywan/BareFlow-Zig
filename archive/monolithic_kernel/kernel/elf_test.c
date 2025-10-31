// ============================================================================
// BAREFLOW - ELF Loader Test
// ============================================================================

#include "elf_loader.h"
#include "vga.h"
#include "stdlib.h"

extern void serial_puts(const char* str);

// Embedded test ELF binary (will be linked in)
extern uint8_t _binary_test_elf_test_module_elf_start[];
extern uint8_t _binary_test_elf_test_module_elf_end[];

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

    // Reverse
    for (int j = 0; j < i/2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = tmp;
    }
    buf[i] = '\0';

    serial_puts(buf);
}

void test_elf_loader(void) {
    serial_puts("\n=== ELF LOADER TEST ===\n");

    // Calculate ELF size
    size_t elf_size = (size_t)(_binary_test_elf_test_module_elf_end -
                                _binary_test_elf_test_module_elf_start);

    serial_puts("[1] ELF binary embedded: ");
    print_int((int)elf_size);
    serial_puts(" bytes\n");

    // Load ELF
    elf_module_t* mod = NULL;
    int result = elf_load(_binary_test_elf_test_module_elf_start, elf_size, NULL, &mod);

    if (result != 0 || mod == NULL) {
        serial_puts("[ERROR] ELF load failed\n");
        return;
    }

    serial_puts("[2] ELF loaded successfully\n");
    serial_puts("    Entry point: 0x");
    // (Simplified - no hex printing)
    serial_puts("\n");
    serial_puts("    Total size: ");
    print_int((int)mod->total_size);
    serial_puts(" bytes\n");

    // Get and execute test_function
    int (*test_func)(void) = (int (*)(void))mod->entry_point;

    serial_puts("[3] Executing test_function()...\n");
    int ret = test_func();

    serial_puts("    Result: ");
    print_int(ret);
    serial_puts("\n");

    if (ret == 42) {
        serial_puts("    \xE2\x9C\x93 PASS: Expected value 42\n");
    } else {
        serial_puts("    [FAIL] Expected 42, got ");
        print_int(ret);
        serial_puts("\n");
    }

    // Clean up
    elf_free(mod);
    serial_puts("[4] ELF module freed\n");

    serial_puts("\n=== ELF LOADER TEST COMPLETE ===\n\n");
}
