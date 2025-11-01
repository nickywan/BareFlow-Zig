// paging.zig - x86-64 4-level page table management
// Session 48 - Phase 5.1: Basic page table setup

const std = @import("std");

/// Page size (4KB)
pub const PAGE_SIZE: usize = 4096;

/// Page table entry flags
pub const PageFlags = packed struct {
    present: bool,           // Bit 0: Page is present
    writable: bool,          // Bit 1: Page is writable
    user: bool,              // Bit 2: User-accessible
    write_through: bool,     // Bit 3: Write-through caching
    cache_disable: bool,     // Bit 4: Disable caching
    accessed: bool,          // Bit 5: Accessed (set by CPU)
    dirty: bool,             // Bit 6: Dirty (set by CPU on write)
    huge: bool,              // Bit 7: Huge page (2MB/1GB)
    global: bool,            // Bit 8: Global page
    available1: u3,          // Bits 9-11: Available for OS
    _reserved: u40 = 0,      // Placeholder for address bits
    available2: u11,         // Bits 52-62: Available for OS
    no_execute: bool,        // Bit 63: No execute (NX)
};

/// Page table entry (64-bit)
pub const PageTableEntry = packed struct(u64) {
    present: u1,             // Bit 0: Page is present
    writable: u1,            // Bit 1: Page is writable
    user: u1,                // Bit 2: User-accessible
    write_through: u1,       // Bit 3: Write-through caching
    cache_disable: u1,       // Bit 4: Disable caching
    accessed: u1,            // Bit 5: Accessed (set by CPU)
    dirty: u1,               // Bit 6: Dirty (set by CPU on write)
    huge: u1,                // Bit 7: Huge page (2MB/1GB)
    global: u1,              // Bit 8: Global page
    available1: u3,          // Bits 9-11: Available for OS
    address: u40,            // Bits 12-51: Physical address >> 12
    available2: u11,         // Bits 52-62: Available for OS
    no_execute: u1,          // Bit 63: No execute (NX)

    /// Create an empty (not present) page table entry
    pub fn empty() PageTableEntry {
        return @bitCast(@as(u64, 0));
    }

    /// Check if entry is present
    pub fn is_present(self: PageTableEntry) bool {
        return self.present == 1;
    }

    /// Get physical address from entry
    pub fn get_address(self: PageTableEntry) usize {
        return @as(usize, self.address) << 12;
    }

    /// Set physical address in entry
    pub fn set_address(self: *PageTableEntry, addr: usize) void {
        self.address = @truncate(addr >> 12);
    }

    /// Create a new page table entry
    pub fn new(phys_addr: usize, writable: bool, user: bool, no_exec: bool) PageTableEntry {
        var entry = PageTableEntry.empty();
        entry.present = 1;
        entry.writable = if (writable) 1 else 0;
        entry.user = if (user) 1 else 0;
        entry.no_execute = if (no_exec) 1 else 0;
        entry.set_address(phys_addr);
        return entry;
    }

    /// Create a new page table entry for MMIO (uncacheable)
    /// Sets PCD (Page Cache Disable) bit for memory-mapped I/O like VGA
    pub fn new_mmio(phys_addr: usize, writable: bool, no_exec: bool) PageTableEntry {
        var entry = PageTableEntry.empty();
        entry.present = 1;
        entry.writable = if (writable) 1 else 0;
        entry.user = 0; // Kernel-only for MMIO
        entry.cache_disable = 1; // PCD=1: Disable caching for MMIO
        entry.no_execute = if (no_exec) 1 else 0;
        entry.set_address(phys_addr);
        return entry;
    }
};

/// Page table (512 entries, 4KB aligned)
pub const PageTable = struct {
    entries: [512]PageTableEntry align(PAGE_SIZE),

    /// Initialize page table with all entries empty
    pub fn init() PageTable {
        var table: PageTable = undefined;
        for (&table.entries) |*entry| {
            entry.* = PageTableEntry.empty();
        }
        return table;
    }

    /// Zero all entries (explicit loop to avoid memset dependency)
    pub fn zero(self: *PageTable) void {
        var i: usize = 0;
        while (i < 512) : (i += 1) {
            self.entries[i] = PageTableEntry.empty();
        }
    }
};

// Static page tables in .data (explicitly zeroed, so GRUB maps them!)
// Note: Must be 4KB aligned
// NOTE (Session 49): Moved from .bss to .data so GRUB maps them!
// This allows us to initialize page tables while using GRUB's mappings.
// Session 50: Increased from 20 to 64 PT tables for better coverage
// 64 PT tables = 64 × 2MB = 128MB address space (covers kernel + heap + growth)
pub var pml4_table: PageTable align(PAGE_SIZE) = PageTable{ .entries = [_]PageTableEntry{PageTableEntry.empty()} ** 512 };
pub var pdpt_table: PageTable align(PAGE_SIZE) = PageTable{ .entries = [_]PageTableEntry{PageTableEntry.empty()} ** 512 };
pub var pd_tables: [4]PageTable align(PAGE_SIZE) = [_]PageTable{PageTable{ .entries = [_]PageTableEntry{PageTableEntry.empty()} ** 512 }} ** 4;
pub var pt_tables: [64]PageTable align(PAGE_SIZE) = [_]PageTable{PageTable{ .entries = [_]PageTableEntry{PageTableEntry.empty()} ** 512 }} ** 64;

// Page table usage tracking
var num_pd_tables: usize = 0;
var num_pt_tables: usize = 0;

/// Get CR3 register (current page table address)
pub fn get_cr3() usize {
    return asm volatile ("mov %%cr3, %[result]"
        : [result] "=r" (-> usize),
    );
}

/// Set CR3 register (load new page tables)
pub fn set_cr3(pml4_addr: usize) void {
    asm volatile ("mov %[pml4], %%cr3"
        :
        : [pml4] "r" (pml4_addr),
        : "memory"
    );
}

/// Flush TLB for a specific virtual address
pub fn flush_tlb(virt_addr: usize) void {
    asm volatile ("invlpg (%[addr])"
        :
        : [addr] "r" (virt_addr),
        : "memory"
    );
}

/// Map a 4KB page with identity mapping
/// phys_addr and virt_addr should be the same for identity mapping
pub fn map_page_identity(virt_addr: usize, writable: bool, no_exec: bool) !void {
    // Extract indices from virtual address
    const pml4_idx = (virt_addr >> 39) & 0x1FF;
    const pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const pd_idx = (virt_addr >> 21) & 0x1FF;
    const pt_idx = (virt_addr >> 12) & 0x1FF;

    // Level 1: PML4 -> PDPT
    if (!pml4_table.entries[pml4_idx].is_present()) {
        pml4_table.entries[pml4_idx] = PageTableEntry.new(
            @intFromPtr(&pdpt_table),
            true, // Writable
            false, // Kernel-only
            false, // Executable (table entries ignore NX)
        );
    }

    // Level 2: PDPT -> PD
    if (!pdpt_table.entries[pdpt_idx].is_present()) {
        if (num_pd_tables >= pd_tables.len) {
            return error.OutOfPageDirectories;
        }
        pd_tables[num_pd_tables].zero();
        pdpt_table.entries[pdpt_idx] = PageTableEntry.new(
            @intFromPtr(&pd_tables[num_pd_tables]),
            true,
            false,
            false,
        );
        num_pd_tables += 1;
    }

    // Get PD table
    const pd_addr = pdpt_table.entries[pdpt_idx].get_address();
    const pd_table = @as(*PageTable, @ptrFromInt(pd_addr));

    // Level 3: PD -> PT
    if (!pd_table.entries[pd_idx].is_present()) {
        if (num_pt_tables >= pt_tables.len) {
            return error.OutOfPageTables;
        }
        pt_tables[num_pt_tables].zero();
        pd_table.entries[pd_idx] = PageTableEntry.new(
            @intFromPtr(&pt_tables[num_pt_tables]),
            true,
            false,
            false,
        );
        num_pt_tables += 1;
    }

    // Get PT table
    const pt_addr = pd_table.entries[pd_idx].get_address();
    const pt_table = @as(*PageTable, @ptrFromInt(pt_addr));

    // Level 4: PT -> Physical page (identity mapping)
    pt_table.entries[pt_idx] = PageTableEntry.new(
        virt_addr & ~@as(usize, 0xFFF), // Align to 4KB
        writable,
        false, // Kernel-only
        no_exec,
    );
}

/// Map a range of pages with identity mapping
pub fn map_range_identity(start: usize, end: usize, writable: bool, no_exec: bool) !void {
    const start_aligned = start & ~@as(usize, PAGE_SIZE - 1);
    const end_aligned = (end + PAGE_SIZE - 1) & ~@as(usize, PAGE_SIZE - 1);

    var addr = start_aligned;
    while (addr < end_aligned) : (addr += PAGE_SIZE) {
        try map_page_identity(addr, writable, no_exec);
    }
}

/// Map a single 4KB page with identity mapping (MMIO/uncacheable version)
/// For memory-mapped I/O like VGA buffer - sets PCD bit
pub fn map_page_mmio(virt_addr: usize, writable: bool, no_exec: bool) !void {
    // Extract indices from virtual address
    const pml4_idx = (virt_addr >> 39) & 0x1FF;
    const pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const pd_idx = (virt_addr >> 21) & 0x1FF;
    const pt_idx = (virt_addr >> 12) & 0x1FF;

    // Level 1: PML4 -> PDPT
    if (!pml4_table.entries[pml4_idx].is_present()) {
        pml4_table.entries[pml4_idx] = PageTableEntry.new(
            @intFromPtr(&pdpt_table),
            true,
            false,
            false,
        );
    }

    // Level 2: PDPT -> PD
    if (!pdpt_table.entries[pdpt_idx].is_present()) {
        if (num_pd_tables >= pd_tables.len) {
            return error.OutOfPageDirectories;
        }
        pd_tables[num_pd_tables].zero();
        pdpt_table.entries[pdpt_idx] = PageTableEntry.new(
            @intFromPtr(&pd_tables[num_pd_tables]),
            true,
            false,
            false,
        );
        num_pd_tables += 1;
    }

    // Get PD table
    const pd_addr = pdpt_table.entries[pdpt_idx].get_address();
    const pd_table = @as(*PageTable, @ptrFromInt(pd_addr));

    // Level 3: PD -> PT
    if (!pd_table.entries[pd_idx].is_present()) {
        if (num_pt_tables >= pt_tables.len) {
            return error.OutOfPageTables;
        }
        pt_tables[num_pt_tables].zero();
        pd_table.entries[pd_idx] = PageTableEntry.new(
            @intFromPtr(&pt_tables[num_pt_tables]),
            true,
            false,
            false,
        );
        num_pt_tables += 1;
    }

    // Get PT table
    const pt_addr = pd_table.entries[pd_idx].get_address();
    const pt_table = @as(*PageTable, @ptrFromInt(pt_addr));

    // Level 4: PT -> Physical page (identity mapping, MMIO/uncacheable)
    pt_table.entries[pt_idx] = PageTableEntry.new_mmio(
        virt_addr & ~@as(usize, 0xFFF), // Align to 4KB
        writable,
        no_exec,
    );
}

/// Map a range of pages with identity mapping (MMIO/uncacheable version)
pub fn map_range_mmio(start: usize, end: usize, writable: bool, no_exec: bool) !void {
    const start_aligned = start & ~@as(usize, PAGE_SIZE - 1);
    const end_aligned = (end + PAGE_SIZE - 1) & ~@as(usize, PAGE_SIZE - 1);

    var addr = start_aligned;
    while (addr < end_aligned) : (addr += PAGE_SIZE) {
        try map_page_mmio(addr, writable, no_exec);
    }
}

// Linker-provided symbols for kernel sections
pub extern var __text_start: u8;
pub extern var __text_end: u8;
pub extern var __rodata_start: u8;
pub extern var __rodata_end: u8;
pub extern var __data_start: u8;
pub extern var __data_end: u8;
pub extern var __bss_start: u8;
pub extern var __bss_end: u8;

/// Initialize paging with custom identity mappings for all kernel sections
/// Session 49: Create custom page tables to map BSS (GRUB doesn't map it!)
/// NOTE: Page tables MUST be in .data (not .bss) so GRUB maps them during bootstrap!
pub fn init_paging() !void {
    // Get kernel section addresses from linker
    const text_start = @intFromPtr(&__text_start);
    const text_end = @intFromPtr(&__text_end);
    const rodata_start = @intFromPtr(&__rodata_start);
    const rodata_end = @intFromPtr(&__rodata_end);
    const data_start = @intFromPtr(&__data_start);
    const data_end = @intFromPtr(&__data_end);
    const bss_start = @intFromPtr(&__bss_start);
    const bss_end = @intFromPtr(&__bss_end);

    // Initialize page table tracking
    num_pd_tables = 0;
    num_pt_tables = 0;

    // Page tables are already zeroed (in .data section, initialized at compile time)

    // Map kernel .text (code) - Read + Execute, no write
    try map_range_identity(text_start, text_end, false, false);

    // Map kernel .rodata (constants) - Read only, no execute
    try map_range_identity(rodata_start, rodata_end, false, true);

    // Map kernel .data (initialized data) - Read + Write, no execute
    try map_range_identity(data_start, data_end, true, true);

    // Map kernel .bss (heap + page tables) - Read + Write, no execute
    // This is the critical mapping GRUB doesn't provide!
    try map_range_identity(bss_start, bss_end, true, true);

    // Map VGA buffer (0xB8000) as MMIO - Read + Write, no execute, uncacheable
    // Session 50: Use map_range_mmio with PCD=1 for proper MMIO handling
    try map_range_mmio(0xB8000, 0xB8000 + 80 * 25 * 2, true, true);

    // Load CR3 with our custom PML4 address
    const pml4_phys = @intFromPtr(&pml4_table);
    set_cr3(pml4_phys);

    // Success - custom page tables now active!
}

/// Dump page table statistics (for debugging)
pub fn dump_stats(print_fn: *const fn ([]const u8) void) void {
    print_fn("Page Table Statistics:\n");
    print_fn("  PD tables used: ");
    // TODO: Add hex printing here
    print_fn("\n  PT tables used: ");
    // TODO: Add hex printing here
    print_fn("\n");
}
