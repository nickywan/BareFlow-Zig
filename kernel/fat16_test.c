// ============================================================================
// FAT16 Filesystem Test
// ============================================================================
// Tests FAT16 read-only filesystem driver with disk I/O
// ============================================================================

#include "fat16.h"
#include "vga.h"
#include "keyboard.h"

// External functions
extern void terminal_writestring(const char* str);
extern void terminal_setcolor(uint8_t fg, uint8_t bg);
extern void terminal_putchar(char c);

// Helper to print numbers
static void print_int(int num) {
    if (num == 0) {
        terminal_putchar('0');
        return;
    }

    if (num < 0) {
        terminal_putchar('-');
        num = -num;
    }

    char buffer[12];
    int i = 0;

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        terminal_putchar(buffer[j]);
    }
}

// Test FAT16 filesystem initialization and file listing
void test_fat16_filesystem(void) {
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writestring("\n=== FAT16 FILESYSTEM TEST ===\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

    // Initialize FAT16 filesystem on secondary IDE drive (slave)
    // QEMU -drive index=1 maps to ATA slave (drive 1)
    // Assume FAT16 partition starts at LBA 0 (entire disk formatted as FAT16)
    fat16_fs_t fs;

    terminal_writestring("[1] Initializing FAT16 filesystem on drive 1 (slave)...\n");
    int result = fat16_init(&fs, 1, 0);  // Drive 1 (ATA slave), partition at LBA 0

    if (result != 0) {
        terminal_setcolor(VGA_LIGHT_RED, VGA_BLACK);
        terminal_writestring("✗ FAT16 initialization failed\n");
        terminal_writestring("  Note: This is expected if no FAT16 disk is attached\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
        return;
    }

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("✓ FAT16 initialized successfully\n\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

#ifdef INTERACTIVE_MODE
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\nPress any key to see filesystem information...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    wait_key();
    terminal_writestring("\n");
#endif

    // Print filesystem information
    terminal_writestring("[2] Filesystem Information:\n");
    terminal_writestring("  Bytes per sector: ");
    print_int(fs.boot_sector.bytes_per_sector);
    terminal_writestring("\n");

    terminal_writestring("  Sectors per cluster: ");
    print_int(fs.boot_sector.sectors_per_cluster);
    terminal_writestring("\n");

    terminal_writestring("  Reserved sectors: ");
    print_int(fs.boot_sector.reserved_sectors);
    terminal_writestring("\n");

    terminal_writestring("  Number of FATs: ");
    print_int(fs.boot_sector.num_fats);
    terminal_writestring("\n");

    terminal_writestring("  Root entries: ");
    print_int(fs.boot_sector.root_entries);
    terminal_writestring("\n");

    terminal_writestring("  Sectors per FAT: ");
    print_int(fs.boot_sector.sectors_per_fat);
    terminal_writestring("\n\n");

#ifdef INTERACTIVE_MODE
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\nPress any key to list files...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    wait_key();
    terminal_writestring("\n");
#endif

    // List files in root directory
    terminal_writestring("[3] Listing files in root directory:\n");
    fat16_list_files(&fs);
    terminal_writestring("\n");

#ifdef INTERACTIVE_MODE
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\nPress any key to test file reading...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    wait_key();
    terminal_writestring("\n");
#endif

    // Try to open and read a test file
    terminal_writestring("[4] Testing file read (TEST.TXT):\n");
    fat16_file_t file;
    result = fat16_open(&fs, "TEST.TXT", &file);

    if (result == 0) {
        terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
        terminal_writestring("✓ File found: TEST.TXT\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

        terminal_writestring("  Size: ");
        print_int(fat16_get_file_size(&file));
        terminal_writestring(" bytes\n");

        // Read first 128 bytes
        char buffer[128];
        int bytes_read = fat16_read(&fs, &file, buffer, 127);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // Null-terminate
            terminal_writestring("  Content: ");
            terminal_writestring(buffer);
            terminal_writestring("\n");
        }

        fat16_close(&file);
    } else {
        terminal_setcolor(VGA_YELLOW, VGA_BLACK);
        terminal_writestring("  File not found (this is OK if TEST.TXT doesn't exist)\n");
        terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    }

    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_writestring("\n✓ FAT16 test complete!\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);

#ifdef INTERACTIVE_MODE
    terminal_setcolor(VGA_YELLOW, VGA_BLACK);
    terminal_writestring("\nPress any key to continue...\n");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    wait_key();
#endif
}
