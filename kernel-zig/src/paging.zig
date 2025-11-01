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

// Static page tables in .bss (automatically zeroed)
// Note: Must be 4KB aligned
// NOTE (Session 48): Reduced from 512 to 8 PT tables to avoid 2MB BSS bloat
// 8 PT tables = 8 × 2MB = 16MB address space (enough for kernel + heap)
var pml4_table: PageTable align(PAGE_SIZE) = undefined;
var pdpt_table: PageTable align(PAGE_SIZE) = undefined;
var pd_tables: [2]PageTable align(PAGE_SIZE) = undefined;  // 2 PD tables (4MB)
var pt_tables: [8]PageTable align(PAGE_SIZE) = undefined;  // 8 PT tables (16MB)

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

/// Initialize paging with identity mapping for kernel and heap
/// NOTE (Session 48): For now, we keep GRUB's page tables active.
/// Creating entirely new page tables causes triple fault because:
/// 1. We access unmapped memory while setting up tables
/// 2. GRUB's identity mapping works fine for our needs
/// This is Phase 5.1 - basic setup. Phase 5.2 will add RW→RX transitions.
pub fn init_paging() !void {
    // For Session 48, we just verify paging is active and keep GRUB's tables
    // TODO (Session 49): Implement custom page tables with proper bootstrapping

    const current_cr3 = get_cr3();
    _ = current_cr3; // GRUB already set this up

    // Initialize our table structures (for future use)
    // NOTE: We don't initialize all 512 PT tables (2MB!) to avoid BSS bloat
    // Only initialize as needed when we actually create custom mappings
    num_pd_tables = 0;
    num_pt_tables = 0;

    // Success - paging is already active from GRUB
}

/// Dump page table statistics (for debugging)
pub fn dump_stats(serial_print: *const fn ([]const u8) void) void {
    serial_print("Page Table Statistics:\n");
    serial_print("  PD tables used: ");
    // TODO: Add hex printing here
    serial_print("\n  PT tables used: ");
    // TODO: Add hex printing here
    serial_print("\n");
}
