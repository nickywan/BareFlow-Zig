/**
 * Profiling Data Export System - Implementation
 */

#include "profiling_export.h"
#include "module_loader.h"

// ============================================================================
// SERIAL PORT (COM1) DRIVER
// ============================================================================

#define COM1_PORT 0x3F8  // COM1 base port

// COM1 registers
#define COM1_DATA       (COM1_PORT + 0)  // Data register (read/write)
#define COM1_INT_ENABLE (COM1_PORT + 1)  // Interrupt enable
#define COM1_FIFO_CTRL  (COM1_PORT + 2)  // FIFO control
#define COM1_LINE_CTRL  (COM1_PORT + 3)  // Line control
#define COM1_MODEM_CTRL (COM1_PORT + 4)  // Modem control
#define COM1_LINE_STATUS (COM1_PORT + 5) // Line status

// Port I/O functions
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int serial_init(void) {
    // Disable interrupts
    outb(COM1_INT_ENABLE, 0x00);

    // Enable DLAB (Divisor Latch Access Bit) to set baud rate
    outb(COM1_LINE_CTRL, 0x80);

    // Set baud rate to 115200 (divisor = 1)
    outb(COM1_DATA, 0x01);        // Low byte
    outb(COM1_INT_ENABLE, 0x00);  // High byte

    // 8 bits, no parity, 1 stop bit, disable DLAB
    outb(COM1_LINE_CTRL, 0x03);

    // Enable FIFO, clear, 14-byte threshold
    outb(COM1_FIFO_CTRL, 0xC7);

    // Enable RTS/DSR
    outb(COM1_MODEM_CTRL, 0x0B);

    // Attempt loopback test, but tolerate failures (some virtual UARTs ignore it)
    outb(COM1_MODEM_CTRL, 0x1E);  // Enable loopback mode
    outb(COM1_DATA, 0xAE);         // Send test byte
    int timeout = 100000;
    while (!(inb(COM1_LINE_STATUS) & 0x01) && --timeout) {
        // spin until data ready
    }
    if (timeout == 0 || inb(COM1_DATA) != 0xAE) {
        outb(COM1_MODEM_CTRL, 0x0F);
        return 0;
    }

    // Disable loopback, enable normal operation
    outb(COM1_MODEM_CTRL, 0x0F);

    serial_puts("[serial] init ok\n");
    return 0;
}

static int serial_is_transmit_empty(void) {
    return inb(COM1_LINE_STATUS) & 0x20;
}

void serial_putchar(char c) {
    int timeout = 100000;
    while (!serial_is_transmit_empty()) {
        if (--timeout == 0) {
            return;
        }
    }

    outb(COM1_DATA, c);
}

void serial_puts(const char* str) {
    while (*str) {
        serial_putchar(*str++);
    }
}

// Helper to reverse a string in place
static void reverse_str(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void serial_put_int(int value) {
    if (value == 0) {
        serial_putchar('0');
        return;
    }

    if (value < 0) {
        serial_putchar('-');
        value = -value;
    }

    char buffer[16];
    int i = 0;

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    reverse_str(buffer, i);
    buffer[i] = '\0';
    serial_puts(buffer);
}

void serial_put_uint(unsigned int value) {
    if (value == 0) {
        serial_putchar('0');
        return;
    }

    char buffer[16];
    int i = 0;

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    reverse_str(buffer, i);
    buffer[i] = '\0';
    serial_puts(buffer);
}

void serial_put_uint64(uint64_t value) {
    if (value == 0) {
        serial_putchar('0');
        return;
    }

    char buffer[32];
    int i = 0;

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    reverse_str(buffer, i);
    buffer[i] = '\0';
    serial_puts(buffer);
}

// ============================================================================
// PROFILING EXPORT
// ============================================================================

uint64_t profiling_get_timestamp(void) {
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

int profiling_export_json(const module_manager_t* mgr) {
    if (!mgr) {
        return -1;
    }

    // Get timestamp
    uint64_t timestamp = profiling_get_timestamp();

    // Start JSON
    serial_puts("{\n");
    serial_puts("  \"format_version\": \"1.0\",\n");
    serial_puts("  \"timestamp_cycles\": ");
    serial_put_uint64(timestamp);
    serial_puts(",\n");
    serial_puts("  \"total_calls\": ");
    serial_put_uint64(mgr->total_calls);
    serial_puts(",\n");
    serial_puts("  \"num_modules\": ");
    serial_put_int(mgr->num_modules);
    serial_puts(",\n");
    serial_puts("  \"modules\": [\n");

    // Export each module
    for (uint32_t i = 0; i < mgr->num_modules; i++) {
        const module_profile_t* mod = &mgr->modules[i];

        serial_puts("    {\n");

        // Module name
        serial_puts("      \"name\": \"");
        serial_puts(mod->name);
        serial_puts("\",\n");

        // Call count
        serial_puts("      \"calls\": ");
        serial_put_uint64(mod->call_count);
        serial_puts(",\n");

        // Total cycles
        serial_puts("      \"total_cycles\": ");
        serial_put_uint64(mod->total_cycles);
        serial_puts(",\n");

        // Min cycles
        uint64_t min_cycles = (mod->call_count == 0) ? 0 : mod->min_cycles;
        serial_puts("      \"min_cycles\": ");
        serial_put_uint64(min_cycles);
        serial_puts(",\n");

        // Max cycles
        uint64_t max_cycles = (mod->call_count == 0) ? 0 : mod->max_cycles;
        serial_puts("      \"max_cycles\": ");
        serial_put_uint64(max_cycles);
        serial_puts(",\n");

        // Note: Average cycles calculated on host as total_cycles / call_count
        // to avoid 64-bit division in bare-metal

        // Code address (as hex string)
        serial_puts("      \"code_address\": \"0x");
        // Simple hex output
        unsigned int addr = (unsigned int)mod->code_ptr;
        for (int shift = 28; shift >= 0; shift -= 4) {
            int nibble = (addr >> shift) & 0xF;
            serial_putchar(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
        }
        serial_puts("\",\n");

        // Code size
        serial_puts("      \"code_size\": ");
        serial_put_uint(mod->code_size);
        serial_puts(",\n");

        // Loaded status
        serial_puts("      \"loaded\": ");
        serial_puts(mod->loaded ? "true" : "false");
        serial_puts("\n");

        serial_puts("    }");
        if (i < mgr->num_modules - 1) {
            serial_putchar(',');
        }
        serial_puts("\n");
    }

    serial_puts("  ]\n");
    serial_puts("}\n");

    return 0;
}

int profiling_trigger_export(const module_manager_t* mgr) {
    // Print header (SERIAL ONLY - no VGA output to avoid mixing)
    serial_puts("\n\n");
    serial_puts("=== PROFILING DATA EXPORT ===\n");
    serial_puts("Format: JSON\n");
    serial_puts("Timestamp: ");
    serial_put_uint64(profiling_get_timestamp());
    serial_puts(" cycles\n");
    serial_puts("--- BEGIN JSON ---\n");

    // Export JSON data
    int result = profiling_export_json(mgr);

    // Print footer
    serial_puts("--- END JSON ---\n");
    serial_puts("\n");
    serial_puts("Workflow:\n");
    serial_puts("1. Save JSON between BEGIN/END markers to file\n");
    serial_puts("2. Feed JSON into host-side PGO tooling (see roadmap task)\n");
    serial_puts("3. Rebuild module cache with optimized binaries\n");
    serial_puts("4. Reassemble kernel image and reboot to load optimizations\n");
    serial_puts("\n=== END EXPORT ===\n\n");

    return result;
}
