/**
 * Serial Port Driver (COM1) - Implementation
 */

#include "serial.h"

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
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int serial_init(void) {
    // Disable all interrupts
    outb(COM1_INT_ENABLE, 0x00);

    // Enable DLAB (set baud rate divisor)
    outb(COM1_LINE_CTRL, 0x80);

    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_DATA, 0x03);
    outb(COM1_INT_ENABLE, 0x00); // (hi byte)

    // 8 bits, no parity, one stop bit
    outb(COM1_LINE_CTRL, 0x03);

    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_FIFO_CTRL, 0xC7);

    // IRQs enabled, RTS/DSR set
    outb(COM1_MODEM_CTRL, 0x0B);

    // Test serial chip by doing loopback test
    outb(COM1_MODEM_CTRL, 0x1E); // Set in loopback mode, test the serial chip
    outb(COM1_DATA, 0xAE);         // Test byte

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(COM1_DATA) != 0xAE) {
        return 1; // Serial faulty
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1_MODEM_CTRL, 0x0F);
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
