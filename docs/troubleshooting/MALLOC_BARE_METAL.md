# Malloc in Bare-Metal: Common Issues & Solutions

**Date**: 2025-10-26
**Session**: 36
**Status**: RESOLVED ✅

---

## 🔴 Problem: "Return Crash" That Wasn't a Return Crash

### Symptoms

```
[TinyLlama] Allocating structure... malloc_OK test_write...write_OK READ_MISMATCH
  ERROR: Model creation failed
```

**Misleading symptoms:**
- Function seemed to "crash on return"
- All code executed (diagnostics printed a, b, c, d, e)
- Crash happened after last instruction before `return 0`

**Actual problem:**
- malloc() returned pointer to **invalid memory**
- Writes succeeded (no segfault)
- Reads returned garbage (memory not actually writable)

---

## 🔍 Root Cause

### Original Implementation (BROKEN)

File: `kernel_lib/memory/malloc_bump.c`

```c
#define HEAP_START_ADDR     0x2100000   // 33 MB - INVALID!
static unsigned long heap_init_flag;
static unsigned char* heap_ptr;

void* malloc(unsigned long size) {
    if (heap_init_flag != HEAP_INIT_MAGIC) {
        heap_ptr = (unsigned char*)HEAP_START_ADDR;  // ← BAD!
        // ...
    }
}
```

**Problem**: Address `0x2100000` (33 MB) was:
- Not identity-mapped in paging tables
- Outside available physical RAM range
- Arbitrary "magic number" with no linker backing

### Why It Manifested as "Return Crash"

1. malloc() returns invalid pointer
2. Code writes to invalid memory (no immediate crash in bare-metal)
3. Code reads back garbage values
4. Function proceeds with corrupted data
5. Eventually hits invalid code path → crash
6. Appears to crash "at the end" (red herring!)

---

## ✅ Solution: Static .bss Heap

File: `tests/phase4/qemu_llvm_64/malloc_simple.c`

```c
#define HEAP_SIZE (64 * 1024 * 1024)  // 64 MB
static uint8_t heap[HEAP_SIZE] __attribute__((aligned(16)));
static size_t heap_offset = 0;

void* malloc(size_t size) {
    if (size == 0) return NULL;

    // Align to 16 bytes
    size = (size + 15) & ~15;

    if (heap_offset + size > HEAP_SIZE)
        return NULL;

    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}
```

### Why This Works

1. **Linker-managed**: Symbol `heap` placed in `.bss` section
2. **Bootloader zeroed**: `.bss` cleared during boot (see `boot.S`)
3. **Identity-mapped**: Linker ensures it's in valid memory region
4. **Deterministic**: Size known at compile-time
5. **No binary bloat**: `.bss` section doesn't increase binary size

---

## 📚 Bare-Metal Memory Management Best Practices

### ✅ DO

1. **Use .bss for heap**
   ```c
   static uint8_t heap[SIZE] __attribute__((aligned(16)));
   ```

2. **Verify memory layout with linker script**
   ```ld
   .bss : {
       _bss_start = .;
       *(.bss)
       _bss_end = .;
   }
   ```

3. **Zero .bss in bootloader**
   ```asm
   movabs $_bss_start, %rdi
   movabs $_bss_end, %rcx
   sub %rdi, %rcx
   xor %eax, %eax
   rep stosb
   ```

4. **Use bump allocator for simplicity**
   - Fast (O(1) allocation)
   - Deterministic
   - No fragmentation concerns for short-lived kernels

### ❌ DON'T

1. **Use arbitrary fixed addresses**
   ```c
   #define HEAP_START 0x2100000  // ❌ May not be valid!
   ```

2. **Assume memory is writable without verification**
   - Just because write doesn't crash ≠ memory is valid
   - Bare-metal has no MMU protection in Ring 0

3. **Use `volatile` unnecessarily**
   ```c
   volatile uint32_t* ptr = ...;  // ❌ Can cause cache issues
   ```
   - In x86-64 bare-metal, `volatile` can interfere with caching
   - Normal writes work fine when memory is valid

4. **Mix 32-bit and 64-bit code**
   ```makefile
   # ❌ kernel_lib compiled with -m32
   # ❌ kernel compiled with -target x86_64-unknown-none
   # Result: Linking errors or runtime corruption
   ```

---

## 🔧 Debugging Checklist

When you see "mysterious crashes":

1. **Verify malloc() returns valid pointers**
   ```c
   void* ptr = malloc(1024);
   if (!ptr) {
       serial_puts("malloc failed\n");
       return -1;
   }

   // Test write/read
   *(uint32_t*)ptr = 0x12345678;
   if (*(uint32_t*)ptr != 0x12345678) {
       serial_puts("Memory not writable!\n");
       return -1;
   }
   ```

2. **Check linker map for memory layout**
   ```bash
   ld -Map=kernel.map ...
   grep "heap" kernel.map
   ```

3. **Verify paging identity-maps all used memory**
   ```c
   // Ensure malloc'd addresses are < max identity-mapped address
   if ((uint64_t)ptr > MAX_IDENTITY_MAPPED) {
       serial_puts("Pointer outside identity-mapped range!\n");
   }
   ```

4. **Test with simple allocations first**
   ```c
   void* p1 = malloc(16);
   void* p2 = malloc(16);
   if (p2 != p1 + 16) {
       serial_puts("malloc not working correctly!\n");
   }
   ```

5. **Check for architecture mismatches**
   ```bash
   file kernel.o  # Should show x86-64
   file lib.a     # Should ALSO show x86-64
   ```

---

## 📊 Real-World Results

### Before (BROKEN - Fixed Address)
```
[TinyLlama] Allocating structure... malloc_OK test_write...write_OK READ_MISMATCH
  ERROR: Model creation failed
```
- Kernel: 28 KB (with 32-bit kernel_lib)
- Success rate: 0%

### After (WORKING - Static .bss)
```
[TinyLlama] Allocating structure... OK
[TinyLlama] Config...12345678 OK
[TinyLlama] Allocating layers array (~5KB)... OK
=== Model created successfully! ===
```
- Kernel: 18 KB (pure 64-bit standalone)
- Success rate: 100%
- Heap: 64 MB static in .bss

---

## 🌐 External Resources

Based on web research (2025-10-26):

### Stack Overflow Consensus

1. **"When does malloc return NULL in bare-metal?"**
   - Answer: When your allocator runs out of heap space
   - Key: You define what "heap" means in bare-metal

2. **"How is BareMetal OS allocating memory without malloc?"**
   - Answer: OS owns all memory, can manage however it wants
   - Solution: Static .bss arrays work perfectly

3. **"Heap vs stack vs bss section"**
   - Answer: .bss is for zero-initialized static/global variables
   - Benefit: No binary size increase, linker-placed

### Best Practice Summary (from web)

> "In bare metal you are the one designing the memory layout, and you can set it up however you want depending on your needs."

> "If global or static variables don't have an initial value, they're put in section .bss to get zero-initialized on load."

> "malloc is generally frowned upon because it's non-deterministic."

**Recommendation**: Use static .bss heap with bump allocator for bare-metal kernels.

---

## 📝 Related Issues

All these issues stemmed from the same malloc bug:

1. **"Return crash" in tinyllama_create_model()** → Invalid malloc pointer
2. **Write OK, Read fails** → Memory not actually writable
3. **Function "crashes at end"** → Misleading symptom
4. **32/64-bit linking errors** → Exposed by fixing malloc (needed pure 64-bit)

**Lesson**: One bad malloc implementation can cause cascading, hard-to-debug issues!

---

## 🎯 Takeaway

**NEVER use fixed addresses for heap in bare-metal unless:**
- You've verified the address is identity-mapped
- You've verified it's in available physical RAM
- You've documented WHY that specific address

**ALWAYS prefer:**
- Static arrays in .bss
- Linker-managed symbols
- Bootloader-zeroed sections
- Simple, deterministic allocators

**REMEMBER:**
> "Bare-metal has no safety net. Invalid memory won't segfault immediately - it'll just give you garbage data and mysterious crashes later."

---

**See also:**
- `malloc_simple.c` (reference implementation)
- `docs/architecture/LLVM_LIBC_STRATEGY.md` (future improvements)
- `docs/phase5/SESSION_36_SUMMARY.md` (debugging chronicle)
