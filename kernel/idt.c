#include <stdint.h>
#include "idt.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern void default_isr(void);

static struct idt_entry idt[256];

static void set_entry(int index, uint32_t offset, uint16_t selector, uint8_t type_attr) {
    idt[index].offset_low = offset & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = type_attr;
    idt[index].offset_high = (offset >> 16) & 0xFFFF;
}

void idt_init(void) {
    for (int i = 0; i < 256; ++i) {
        set_entry(i, (uint32_t)default_isr, 0x08, 0x8E);
    }

    struct idt_ptr ptr;
    ptr.limit = sizeof(idt) - 1;
    ptr.base = (uint32_t)idt;

    asm volatile ("lidt %0" : : "m"(ptr));
}
