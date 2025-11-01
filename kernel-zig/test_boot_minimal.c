// Ultra-minimal multiboot2 test kernel
// Just writes "TEST!" to VGA and halts

__attribute__((section(".multiboot")))
__attribute__((aligned(8)))
struct {
    unsigned int magic;
    unsigned int architecture;
    unsigned int header_length;
    unsigned int checksum;

    // End tag
    unsigned short type;
    unsigned short flags;
    unsigned int size;
} multiboot_header = {
    0xE85250D6,  // magic
    0,           // architecture (i386)
    24,          // header_length
    -(0xE85250D6 + 0 + 24),  // checksum

    0,   // end tag type
    0,   // end tag flags
    8    // end tag size
};

void _start() {
    // Write "TEST!" to VGA at 0xB8000
    unsigned short *vga = (unsigned short *)0xB8000;
    vga[0] = 0x0F54;  // 'T'
    vga[1] = 0x0F45;  // 'E'
    vga[2] = 0x0F53;  // 'S'
    vga[3] = 0x0F54;  // 'T'
    vga[4] = 0x0F21;  // '!'

    // Halt forever
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}
