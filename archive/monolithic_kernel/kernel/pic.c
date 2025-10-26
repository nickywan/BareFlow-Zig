// 8259 PIC (Programmable Interrupt Controller) driver
#include <stdint.h>
#include "pic.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pic_init(void) {
    // ICW1: Start initialization
    outb(PIC1_CMD, ICW1_INIT);
    outb(PIC2_CMD, ICW1_INIT);

    // ICW2: Remap IRQs (0x20-0x27 for PIC1, 0x28-0x2F for PIC2)
    outb(PIC1_DATA, 0x20);  // Master PIC offset
    outb(PIC2_DATA, 0x28);  // Slave PIC offset

    // ICW3: Cascading (IRQ2 connects to slave)
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);

    // ICW4: 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // Mask all IRQs initially (except IRQ2 cascade)
    outb(PIC1_DATA, 0xFB);  // 11111011 - allow IRQ2
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(int irq) {
    if (irq >= 8)
        outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}

void pic_unmask_irq(int irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t mask = inb(port);
    mask &= ~(1 << (irq % 8));
    outb(port, mask);
}
