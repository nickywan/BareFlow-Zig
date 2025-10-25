#include <stdint.h>
#include "idt.h"
#include "../io/vga.h"

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

// External ISR handlers from idt_stub.asm
extern void default_isr(void);
extern void isr_div_zero(void);
extern void isr_debug(void);
extern void isr_nmi(void);
extern void isr_breakpoint(void);
extern void isr_overflow(void);
extern void isr_bound(void);
extern void isr_invalid_op(void);
extern void isr_gpf(void);
extern void isr_page_fault(void);
extern void irq_timer(void);
extern void irq_keyboard(void);

static struct idt_entry idt[256];
static volatile uint32_t timer_ticks = 0;

static void set_entry(int index, uint32_t offset, uint16_t selector, uint8_t type_attr) {
    idt[index].offset_low = offset & 0xFFFF;
    idt[index].selector = selector;
    idt[index].zero = 0;
    idt[index].type_attr = type_attr;
    idt[index].offset_high = (offset >> 16) & 0xFFFF;
}

// Exception handler called from assembly
void exception_handler(void) {
    terminal_setcolor(VGA_RED, VGA_BLACK);
    terminal_writestring("\n[EXCEPTION] CPU Exception caught!\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    while(1) { asm("hlt"); }  // Halt
}

// Timer IRQ handler
void timer_handler(void) {
    extern void pic_send_eoi(int);
    timer_ticks++;
    pic_send_eoi(0);
}

// Keyboard IRQ handler (stub)
void keyboard_handler(void) {
    extern void pic_send_eoi(int);
    pic_send_eoi(1);
}

void idt_init(void) {
    // Initialize all to default
    for (int i = 0; i < 256; ++i) {
        set_entry(i, (uint32_t)default_isr, 0x08, 0x8E);
    }

    // Set specific exception handlers
    set_entry(0, (uint32_t)isr_div_zero, 0x08, 0x8E);
    set_entry(1, (uint32_t)isr_debug, 0x08, 0x8E);
    set_entry(2, (uint32_t)isr_nmi, 0x08, 0x8E);
    set_entry(3, (uint32_t)isr_breakpoint, 0x08, 0x8E);
    set_entry(4, (uint32_t)isr_overflow, 0x08, 0x8E);
    set_entry(5, (uint32_t)isr_bound, 0x08, 0x8E);
    set_entry(6, (uint32_t)isr_invalid_op, 0x08, 0x8E);
    set_entry(13, (uint32_t)isr_gpf, 0x08, 0x8E);
    set_entry(14, (uint32_t)isr_page_fault, 0x08, 0x8E);

    // IRQ handlers (remapped to 0x20-0x2F)
    set_entry(0x20, (uint32_t)irq_timer, 0x08, 0x8E);     // IRQ0
    set_entry(0x21, (uint32_t)irq_keyboard, 0x08, 0x8E); // IRQ1

    struct idt_ptr ptr;
    ptr.limit = sizeof(idt) - 1;
    ptr.base = (uint32_t)idt;

    asm volatile ("lidt %0" : : "m"(ptr));
}
