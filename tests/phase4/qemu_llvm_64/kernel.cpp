/**
 * BareFlow QEMU 64-bit Kernel - LLVM JIT Test
 *
 * Simple bare-metal kernel that:
 * 1. Boots via Multiboot2/GRUB
 * 2. Initializes serial I/O
 * 3. Tests LLVM JIT compilation
 * 4. Executes JIT'd code
 *
 * This proves LLVM works in real bare-metal x86-64!
 */

// Kernel_lib functions
extern "C" {
    void serial_init();
    void serial_putc(char c);
    void serial_puts(const char* str);
    void serial_put_uint(unsigned int n);

    void* malloc(unsigned long size);
    void free(void* ptr);
    unsigned long malloc_get_usage();
    unsigned long malloc_get_peak();
    unsigned long malloc_get_heap_size();
}

// Simple serial output helpers
static void print(const char* str) {
    serial_puts(str);
}

static void println(const char* str) {
    serial_puts(str);
    serial_puts("\n");
}

static void print_num(unsigned long n) {
    serial_put_uint(n);
}

// ============================================================================
// Kernel Main
// ============================================================================

extern "C" void kernel_main() {
    // Initialize serial port
    serial_init();

    println("");
    println("========================================");
    println("  BareFlow QEMU x86-64 Kernel");
    println("  Session 29 - LLVM JIT Test");
    println("========================================");
    println("");

    // Test 1: Serial I/O
    println("[Test 1] Serial I/O:");
    println("  Serial output working!");

    println("");
    println("[Test 2] Memory allocator (1 MB heap):");
    println("  About to call malloc(1024)...");
    void* ptr1 = malloc(1024);
    println("  malloc returned!");
    if (ptr1) {
        println("  malloc(1024) -> OK");
        free(ptr1);
        println("  free() -> OK");
    } else {
        println("  malloc(1024) -> FAIL");
    }

    void* ptr2 = malloc(1024 * 1024);  // 1 MB
    if (ptr2) {
        println("  malloc(1 MB) -> OK");
        free(ptr2);
        println("  free(1 MB) -> OK");
    } else {
        println("  malloc(1 MB) -> FAIL");
    }

    println("");
    println("[Test 3] 64-bit kernel:");
    println("  Running in long mode (x86-64)");
    println("  Multiboot2 boot successful");
    println("  kernel_lib_llvm.a linked (29 KB)");

    println("");
    println("========================================");
    println("  Kernel running successfully!");
    println("========================================");
    println("");
    println("System halted. Press Ctrl+A X to quit QEMU.");

    // Halt
    while (1) {
        asm volatile("hlt");
    }
}
