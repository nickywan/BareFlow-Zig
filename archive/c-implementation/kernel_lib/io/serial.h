/**
 * Serial Port Driver (COM1)
 *
 * Basic serial port interface for output via COM1 (0x3F8).
 * Baud rate: 115200, 8N1 (8 bits, no parity, 1 stop bit)
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize serial port (COM1) for output
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

#ifdef __cplusplus
}
#endif

#endif // SERIAL_H
