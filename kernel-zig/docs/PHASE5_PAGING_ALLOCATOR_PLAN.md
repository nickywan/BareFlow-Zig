# Phase 5: Paging & Advanced Allocator Implementation Plan

**Session**: 48+
**Status**: 📋 PLANNING
**Goal**: Enable LLVM ORC JIT and TinyLlama integration with proper memory management

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Implementation Steps](#implementation-steps)
4. [Code Design](#code-design)
5. [Testing Strategy](#testing-strategy)
6. [Risks & Mitigations](#risks--mitigations)
7. [Timeline](#timeline)

---

## Overview

### Current State (Session 47 Complete)

✅ **Working**:
- Native 64-bit boot (boot64.S + Zig kernel)
- 32MB static heap (simple bump allocator)
- Serial I/O + VGA output
- All critical bugs resolved

❌ **Missing** (blocking LLVM ORC JIT):
- Page table management (currently GRUB's identity mapping)
- RW→RX transitions (JIT needs writable-then-executable pages)
- Advanced allocator (free-list, regions, alignment)
- Memory protection (prevent accidental overwrites)

### Why This Phase is Critical

**LLVM ORC JIT Requirements** (from research):
1. **Allocate RW pages** → Write JIT compiled code
2. **Switch to RX** → Execute code (W^X security)
3. **Small code model** → Code + data within 4GB range
4. **finalizeMemory()** → Apply final permissions before execution
5. **SectionMemoryManager** → Separate code/data sections

**TinyLlama Requirements**:
- ~60MB model weights (need efficient allocation)
- Matrix operations (16-byte alignment for SIMD)
- Large contiguous blocks

---

## Architecture

### Memory Map (Physical + Virtual)

```
Physical Memory (Identity Mapped):
0x0000_0000 - 0x0000_0FFF    Null page (unmapped for safety)
0x0000_1000 - 0x0009_FFFF    Low memory (BIOS, VGA)
0x000A_0000 - 0x000B_7FFF    VGA buffer
0x000B_8000 - 0x000B_8FA0    VGA text mode (80×25×2)
0x0010_0000 - 0x01FF_FFFF    Kernel code + data (2MB)
0x0200_0000 - 0x03FF_FFFF    Kernel heap (32MB current)
0x0400_0000 - 0x07FF_FFFF    JIT code region (64MB)
0x0800_0000 - 0x0FFF_FFFF    Model data region (128MB for TinyLlama)
0x1000_0000+                 Free physical memory

Virtual Memory (Higher-Half Kernel - Optional):
0x0000_0000_0000_0000        Null (unmapped)
0x0000_0000_0010_0000        Identity-mapped kernel (2MB)
0x0000_0000_0200_0000        Identity-mapped heap (32MB)
0x0000_0000_0400_0000        Identity-mapped JIT region (64MB)
0xFFFF_FFFF_8000_0000        Higher-half kernel (future)
```

### Page Table Structure (x86-64 4-Level)

```
Virtual Address (48-bit):
[47:39] PML4 index (9 bits) → 512 entries
[38:30] PDPT index (9 bits) → 512 entries
[29:21] PD index   (9 bits) → 512 entries
[20:12] PT index   (9 bits) → 512 entries
[11:0]  Offset     (12 bits) → 4KB pages

Page Table Entry (64-bit):
Bit 0:     Present (P)
Bit 1:     Read/Write (RW)
Bit 2:     User/Supervisor (US)
Bit 3:     Page-level Write-Through (PWT)
Bit 4:     Page-level Cache Disable (PCD)
Bit 5:     Accessed (A)
Bit 6:     Dirty (D)
Bit 7:     Page Size (PS) - 0 for 4KB
Bit 8:     Global (G)
Bits 9-11: Available for OS use
Bits 12-51: Physical address (4KB aligned)
Bit 63:    Execute Disable (NX)

Permissions:
RW=0, NX=1: Read-only, no execute
RW=1, NX=1: Read-write, no execute (writable data)
RW=0, NX=0: Read-only, executable (code)
RW=1, NX=0: Read-write-execute (RWX - avoid if possible)
```

### Allocator Regions

```zig
// Memory region types
const RegionType = enum {
    kernel_heap,    // General allocations (malloc-like)
    jit_code,       // JIT compiled code (RW→RX)
    model_data,     // TinyLlama weights (read-mostly)
    page_tables,    // Page table structures
};

// Memory region descriptor
const MemoryRegion = struct {
    base: usize,          // Virtual base address
    size: usize,          // Total size in bytes
    used: usize,          // Currently allocated
    region_type: RegionType,
    permissions: PagePerms,
};
```

---

## Implementation Steps

### Phase 5.1: Basic Page Table Setup (Session 48)

**Goal**: Replace GRUB's identity mapping with our own page tables

**Tasks**:
1. Create page table structures in Zig
2. Initialize PML4, PDPT, PD, PT tables
3. Identity-map kernel code (0x100000 - 0x1FFFFF)
4. Identity-map heap (0x200000 - 0x3FFFFFF)
5. Load CR3 with our PML4
6. Validate with QEMU (check no triple fault)

**Files**:
- `src/paging.zig` (new)
- `src/main.zig` (call paging_init())
- `src/boot64.S` (may need adjustments)

**Success Criteria**:
- Kernel boots successfully
- Serial output shows "Paging initialized"
- No triple faults
- Can access VGA buffer at 0xB8000

---

### Phase 5.2: RW→RX Transitions (Session 49)

**Goal**: Enable write-then-execute pattern for JIT code

**Tasks**:
1. Implement `map_page()` function
2. Implement `set_page_permissions()` function
3. Create test: allocate RW page, write code, switch to RX
4. Validate NX bit support (check CPUID)
5. Test with simple shellcode (ret instruction)

**Code Sample**:
```zig
// Map a page with specific permissions
pub fn map_page(
    virt_addr: usize,
    phys_addr: usize,
    perms: PagePerms
) !void {
    const pml4_idx = (virt_addr >> 39) & 0x1FF;
    const pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const pd_idx = (virt_addr >> 21) & 0x1FF;
    const pt_idx = (virt_addr >> 12) & 0x1FF;

    // Walk page tables, create if missing
    // Set PTE with permissions
}

// Change page permissions (RW→RX for JIT)
pub fn set_page_permissions(
    virt_addr: usize,
    perms: PagePerms
) !void {
    // Find PTE
    // Update RW and NX bits
    // Flush TLB with invlpg
}
```

**Success Criteria**:
- Can allocate page as RW
- Can write JIT code to page
- Can switch page to RX
- Can execute code from page
- Page fault if write after RX

---

### Phase 5.3: Advanced Allocator - Free List (Session 50)

**Goal**: Replace bump allocator with free-list allocator

**Tasks**:
1. Design free-list structure (linked list of free blocks)
2. Implement `alloc()` with best-fit strategy
3. Implement `free()` with coalescing
4. Add alignment support (4KB, 16-byte, etc.)
5. Add region tracking (kernel heap vs JIT vs model data)

**Code Sample**:
```zig
// Free block header (in-band metadata)
const FreeBlock = struct {
    size: usize,           // Block size (including header)
    next: ?*FreeBlock,     // Next free block in list
    magic: u32 = 0xDEADBEEF,  // Canary for corruption detection
};

// Allocator state
const Allocator = struct {
    free_list: ?*FreeBlock,
    heap_start: usize,
    heap_end: usize,
    total_allocated: usize,

    pub fn alloc(self: *Allocator, size: usize, alignment: usize) !*u8 {
        // Search free list for suitable block
        // Split block if too large
        // Return aligned pointer
    }

    pub fn free(self: *Allocator, ptr: *u8) void {
        // Find block header
        // Add to free list
        // Coalesce with adjacent free blocks
    }
};
```

**Success Criteria**:
- Can allocate variable-sized blocks
- Can free blocks
- Coalescing works (no fragmentation in simple tests)
- Alignment respected (4KB, 16-byte)
- No memory corruption (canary checks)

---

### Phase 5.4: Region-Based Allocation (Session 51)

**Goal**: Separate heap, JIT code, and model data

**Tasks**:
1. Define memory regions in memory map
2. Create region allocator wrapper
3. Implement JIT code allocator (RW→RX support)
4. Implement model data allocator (large blocks)
5. Add region statistics tracking

**Code Sample**:
```zig
const RegionAllocator = struct {
    regions: [4]MemoryRegion,

    // Allocate from specific region
    pub fn alloc_from_region(
        self: *RegionAllocator,
        region_type: RegionType,
        size: usize
    ) !*u8 {
        const region = &self.regions[@intFromEnum(region_type)];
        // Allocate from region's free list
    }

    // JIT-specific allocator (RW initially)
    pub fn alloc_jit_code(self: *RegionAllocator, size: usize) !*u8 {
        const ptr = try self.alloc_from_region(.jit_code, size);
        // Map as RW initially
        try map_pages_rw(@intFromPtr(ptr), size);
        return ptr;
    }

    // Finalize JIT code (RW→RX)
    pub fn finalize_jit_code(ptr: *u8, size: usize) !void {
        try map_pages_rx(@intFromPtr(ptr), size);
    }
};
```

**Success Criteria**:
- JIT code allocated in dedicated region
- Model data allocated in separate region
- Can switch JIT pages from RW to RX
- Regions don't overlap
- Can track per-region usage

---

### Phase 5.5: Zig Allocator Interface (Session 52)

**Goal**: Integrate with Zig's std.mem.Allocator

**Tasks**:
1. Implement std.mem.Allocator vtable
2. Wrap region allocator
3. Test with Zig's ArrayList, HashMap
4. Add error handling (OutOfMemory)
5. Add debug allocator (bounds checking)

**Code Sample**:
```zig
// Zig allocator wrapper
pub const kernel_allocator = std.mem.Allocator{
    .ptr = &global_region_allocator,
    .vtable = &allocator_vtable,
};

const allocator_vtable = std.mem.Allocator.VTable{
    .alloc = alloc_fn,
    .resize = resize_fn,
    .free = free_fn,
};

fn alloc_fn(
    ctx: *anyopaque,
    len: usize,
    ptr_align: u8,
    ret_addr: usize
) ?[*]u8 {
    _ = ret_addr;
    const allocator = @as(*RegionAllocator, @ptrCast(@alignCast(ctx)));
    const alignment = @as(usize, 1) << @intCast(ptr_align);
    return allocator.alloc_from_region(.kernel_heap, len, alignment) catch null;
}
```

**Success Criteria**:
- Works with Zig's ArrayList
- Works with Zig's HashMap
- OutOfMemory errors handled
- No memory leaks (all freed blocks reusable)

---

### Phase 5.6: LLVM ORC JIT Integration Prep (Session 53)

**Goal**: Prepare memory manager for LLVM ORC JIT

**Tasks**:
1. Research LLVM SectionMemoryManager API
2. Design C++ wrapper for Zig allocator
3. Implement allocateCodeSection() equivalent
4. Implement allocateDataSection() equivalent
5. Implement finalizeMemory() (RW→RX)

**Code Sample (C++ wrapper)**:
```cpp
// C++ wrapper for Zig allocator (to be called from LLVM)
extern "C" {
    void* zig_alloc_code_section(size_t size, unsigned alignment);
    void* zig_alloc_data_section(size_t size, unsigned alignment);
    void zig_finalize_memory();
    void zig_free_section(void* ptr);
}

class ZigMemoryManager : public llvm::SectionMemoryManager {
public:
    uint8_t* allocateCodeSection(
        uintptr_t size,
        unsigned alignment,
        unsigned sectionID,
        StringRef sectionName
    ) override {
        return (uint8_t*)zig_alloc_code_section(size, alignment);
    }

    uint8_t* allocateDataSection(
        uintptr_t size,
        unsigned alignment,
        unsigned sectionID,
        StringRef sectionName,
        bool isReadOnly
    ) override {
        return (uint8_t*)zig_alloc_data_section(size, alignment);
    }

    bool finalizeMemory(std::string *errMsg) override {
        zig_finalize_memory();
        return false; // no error
    }
};
```

**Success Criteria**:
- C++ wrapper compiles
- Can call Zig allocator from C++
- LLVM can allocate code sections
- finalizeMemory() switches pages to RX
- No memory leaks

---

## Code Design

### File Structure

```
src/
├── paging.zig          # Page table management
│   ├── map_page()
│   ├── unmap_page()
│   ├── set_page_permissions()
│   └── init_paging()
│
├── allocator.zig       # Advanced allocator
│   ├── FreeBlock
│   ├── Allocator
│   └── alloc() / free()
│
├── region_allocator.zig # Region-based wrapper
│   ├── RegionAllocator
│   ├── alloc_jit_code()
│   ├── finalize_jit_code()
│   └── alloc_model_data()
│
├── zig_allocator.zig   # std.mem.Allocator interface
│   └── kernel_allocator
│
└── jit_memory.cpp      # LLVM ORC JIT wrapper (C++)
    └── ZigMemoryManager
```

### Key Data Structures

```zig
// Page permissions
const PagePerms = struct {
    present: bool = true,
    writable: bool = false,
    user: bool = false,
    executable: bool = false, // NX=!executable
};

// Page table entry
const PageTableEntry = packed struct {
    present: u1,
    writable: u1,
    user: u1,
    write_through: u1,
    cache_disable: u1,
    accessed: u1,
    dirty: u1,
    page_size: u1,
    global: u1,
    available: u3,
    address: u40,  // Physical address >> 12
    reserved: u11,
    nx: u1,
};

// Page table (512 entries)
const PageTable = struct {
    entries: [512]PageTableEntry align(4096),
};
```

---

## Testing Strategy

### Unit Tests

```zig
test "page table entry creation" {
    const pte = PageTableEntry{
        .present = 1,
        .writable = 1,
        .address = 0x1000 >> 12,
        .nx = 1,
        // ... other fields
    };
    try expect(pte.present == 1);
    try expect(pte.get_physical_address() == 0x1000);
}

test "allocator - alloc and free" {
    var allocator = Allocator.init(heap_start, heap_size);
    const ptr1 = try allocator.alloc(1024, 16);
    const ptr2 = try allocator.alloc(2048, 16);
    allocator.free(ptr1);
    const ptr3 = try allocator.alloc(512, 16); // Should reuse ptr1's space
    try expect(ptr3 == ptr1); // Reused freed block
}

test "region allocator - JIT code RW→RX" {
    var region_alloc = RegionAllocator.init();
    const code_ptr = try region_alloc.alloc_jit_code(4096);

    // Write code (should work - RW)
    code_ptr[0] = 0xC3; // ret instruction

    // Finalize (RW→RX)
    try region_alloc.finalize_jit_code(code_ptr, 4096);

    // Execute code (should work - RX)
    const func = @as(*const fn() void, @ptrCast(code_ptr));
    func();

    // Write code (should page fault - RX only)
    // code_ptr[0] = 0x90; // ← This should fault
}
```

### Integration Tests (QEMU)

1. **Test 1: Basic paging**
   - Boot with custom page tables
   - Verify no triple faults
   - Access VGA buffer
   - Serial output works

2. **Test 2: RW→RX JIT test**
   - Allocate JIT page (RW)
   - Write shellcode: `mov $42, %eax; ret`
   - Switch to RX
   - Call function
   - Verify return value = 42

3. **Test 3: Large allocations (TinyLlama)**
   - Allocate 60MB block
   - Fill with test data
   - Verify no corruption
   - Free and reallocate

4. **Test 4: Fragmentation stress test**
   - Allocate 1000 blocks (varying sizes)
   - Free every other block
   - Reallocate (should coalesce)
   - Verify no excessive fragmentation

### Debugging Tools

```zig
// Page table walker (for debugging)
pub fn dump_page_tables(virt_addr: usize) void {
    serial_print("Page table walk for ");
    serial_print_hex64(virt_addr);
    serial_print("\n");

    const pml4_idx = (virt_addr >> 39) & 0x1FF;
    const pdpt_idx = (virt_addr >> 30) & 0x1FF;
    const pd_idx = (virt_addr >> 21) & 0x1FF;
    const pt_idx = (virt_addr >> 12) & 0x1FF;

    serial_print("  PML4[");
    serial_print_hex(pml4_idx);
    serial_print("] = ...\n");
    // ... walk and print each level
}

// Allocator statistics
pub fn dump_allocator_stats(allocator: *Allocator) void {
    serial_print("Allocator Stats:\n");
    serial_print("  Total: ");
    serial_print_hex(allocator.heap_end - allocator.heap_start);
    serial_print("\n  Allocated: ");
    serial_print_hex(allocator.total_allocated);
    serial_print("\n  Free: ");
    serial_print_hex(allocator.heap_end - allocator.heap_start - allocator.total_allocated);
    serial_print("\n");
}
```

---

## Risks & Mitigations

### Risk 1: Triple Faults (HIGH)

**Symptom**: CPU resets during page table setup

**Causes**:
- Invalid page table entries
- Missing present bit
- Misaligned page tables (must be 4KB aligned)
- Recursive page faults

**Mitigation**:
1. Validate all PTEs before loading CR3
2. Use QEMU `-d int,cpu_reset` for debugging
3. Keep GRUB's identity mapping initially
4. Add comprehensive logging before each step
5. Test with simple identity mapping first

**Debugging**:
```bash
# QEMU with interrupt debugging
qemu-system-x86_64 -cdrom kernel.iso \
    -d int,cpu_reset,guest_errors \
    -no-reboot \
    -serial file:serial.log
```

### Risk 2: Memory Corruption (MEDIUM)

**Symptom**: Random crashes, garbage data

**Causes**:
- Free-list corruption
- Double-free bugs
- Use-after-free
- Buffer overflows into metadata

**Mitigation**:
1. Magic canaries in free blocks (0xDEADBEEF)
2. Validate free list on each operation
3. Zero freed memory (debug mode)
4. Bounds checking allocator (debug mode)
5. Separate metadata from user data

**Code**:
```zig
// Validate free list integrity
fn validate_free_list(allocator: *Allocator) bool {
    var current = allocator.free_list;
    var count: usize = 0;
    while (current) |block| {
        if (block.magic != 0xDEADBEEF) {
            serial_print("ERROR: Corrupted free block!\n");
            return false;
        }
        current = block.next;
        count += 1;
        if (count > 10000) {
            serial_print("ERROR: Free list loop detected!\n");
            return false;
        }
    }
    return true;
}
```

### Risk 3: Fragmentation (MEDIUM)

**Symptom**: Out of memory despite free space

**Causes**:
- Poor allocation strategy
- No coalescing of adjacent blocks
- Small free blocks unusable

**Mitigation**:
1. Implement coalescing (merge adjacent free blocks)
2. Use best-fit or first-fit strategy
3. Monitor fragmentation metrics
4. Consider buddy allocator (future)

### Risk 4: W^X Violations (LOW)

**Symptom**: Security vulnerability (RWX pages)

**Causes**:
- Forgot to set NX bit
- RWX pages for JIT (convenience)

**Mitigation**:
1. Always set NX bit for data pages
2. Never use RWX (always RW→RX transition)
3. Audit all page mappings
4. Add assertion checks

---

## Timeline

### Session 48 (Week 1)
- **Phase 5.1**: Basic page table setup
- **Deliverable**: Identity-mapped kernel with custom page tables

### Session 49 (Week 1)
- **Phase 5.2**: RW→RX transitions
- **Deliverable**: Working JIT shellcode test

### Session 50 (Week 2)
- **Phase 5.3**: Free-list allocator
- **Deliverable**: alloc()/free() with coalescing

### Session 51 (Week 2)
- **Phase 5.4**: Region-based allocation
- **Deliverable**: Separate heap/JIT/model regions

### Session 52 (Week 3)
- **Phase 5.5**: Zig allocator interface
- **Deliverable**: Works with ArrayList/HashMap

### Session 53 (Week 3)
- **Phase 5.6**: LLVM ORC JIT prep
- **Deliverable**: C++ wrapper + finalizeMemory()

**Total**: 3 weeks, 6 sessions

---

## Success Criteria (Phase 5 Complete)

- ✅ Custom page tables loaded (no GRUB mapping)
- ✅ Can allocate pages with specific permissions
- ✅ RW→RX transitions work for JIT code
- ✅ Free-list allocator with coalescing
- ✅ Region-based allocation (heap/JIT/model)
- ✅ Zig std.mem.Allocator interface
- ✅ LLVM ORC JIT memory manager wrapper
- ✅ All tests pass (unit + integration)
- ✅ No triple faults in QEMU
- ✅ No memory corruption detected

---

## References

- **LLVM ORC JIT**: https://llvm.org/docs/ORCv2.html
- **JITLink**: https://llvm.org/docs/JITLink.html
- **SectionMemoryManager**: https://llvm.org/doxygen/classllvm_1_1SectionMemoryManager.html
- **x86-64 Paging**: https://wiki.osdev.org/Paging
- **Zig Allocators**: https://ziglang.org/documentation/master/std/#A;std:mem.Allocator

---

**Created**: 2025-11-01 (Session 47 continuation)
**Status**: Planning complete, ready for implementation
**Next**: Session 48 - Phase 5.1 (Basic page table setup)
