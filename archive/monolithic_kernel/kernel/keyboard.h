// kernel/keyboard.h - Simple keyboard input for testing

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Read a byte from keyboard controller
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Check if key is available
static inline int keyboard_has_key(void) {
    return inb(0x64) & 0x01;
}

// Read scancode from keyboard
static inline uint8_t keyboard_read(void) {
    while (!keyboard_has_key()) {
        asm volatile("pause");
    }
    return inb(0x60);
}

// Wait for any key press (consumes the key)
static inline void wait_key(void) {
    // Flush keyboard buffer first
    while (keyboard_has_key()) {
        inb(0x60);
    }

    // Wait for new key
    keyboard_read();

    // Flush again (consume key release)
    while (keyboard_has_key()) {
        inb(0x60);
    }
}

#endif // KEYBOARD_H
