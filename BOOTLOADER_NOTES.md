# Bootloader Capacity Analysis

**Date**: 2025-10-25
**Status**: 128 sectors (64KB) - SUFFICIENT for current needs

---

## Current Configuration

- **Stage 1**: 512 bytes (1 sector) - MBR bootloader
- **Stage 2**: 4KB (8 sectors) - Extended bootloader with LBA/CHS support
- **Kernel Load Capacity**: 128 sectors (64KB)
- **Current Kernel Size**: 57KB (112 sectors)
- **Remaining Capacity**: 7KB (16 sectors)

---

## Why 128 Sectors is Sufficient

### Current Project State
1. **Kernel binary**: 57,024 bytes (112 sectors)
2. **Components included**:
   - Core kernel + VGA + stdlib + keyboard
   - Module loader + profiling system
   - JIT allocator (CODE/DATA/METADATA pools)
   - Micro-JIT (fibonacci, sum patterns)
   - Adaptive JIT with atomic code swapping
   - Function profiler
   - FAT16 filesystem driver
   - Bitcode module loader
   - C++ runtime stubs
   - llvm-libc (8 functions)
   - 12 embedded benchmark modules

3. **Safety margin**: 7KB available for future additions

### User Priority Confirmed
> "ce n'est pas la taille du kernel qui est déteminant, c'est plus les opitmisation à la volée"
> (It's not kernel size that matters, it's runtime optimization)

**Key insight**: We don't need to statically link LLVM into the kernel. Instead:
- Load bitcode modules from disk (FAT16)
- JIT compile at runtime with LLVM backend
- Keep kernel small and modular

---

## Bootloader Technical Notes

### LBA Mode Load Logic (boot/stage2.asm:190-234)
```asm
load_kernel_lba:
    mov cx, 16                      ; 16 iterations
    mov bx, 0                       ; Starting offset
    mov ax, 0x1000                  ; Starting segment (0x1000:0x0000 = 0x10000)
    mov di, 9                       ; Starting LBA sector
.loop:
    ; Read 8 sectors per iteration
    ; ... DAP setup ...
    int 0x13                        ; BIOS disk read
    add di, 8                       ; Next LBA
    add bx, 8 * 512                 ; Advance offset by 4096 bytes
    loop .loop
```

**Key constraint**: Each iteration reads 8 sectors (4096 bytes) and advances `bx` by 4096.
- After 16 iterations: 16 × 4096 = 64KB loaded
- `bx` wraps around at 64KB (0x10000), which works because we're incrementing within a single segment

### Why 256 Sectors Failed (Attempted Fix)
When trying to increase to 256 sectors (32 iterations):
- Changed `mov cx, 32`
- Problem: After 16 iterations, `bx` would wrap (0xFFFF → 0x0000)
- Attempted fix: Increment segment register (`add ax, 0x100`) instead of offset
- **Bug**: Removed `push ax`/`pop ax` but still modified `ax`, breaking segment base
- **Result**: Bootloader loaded kernel to wrong memory locations, causing crash

### Proper Fix for >128 Sectors (Future Work)
If we need >64KB capacity later, the correct approach:

**Option A: Multiple segment loading**
```asm
load_kernel_lba:
    mov cx, 32                      ; 32 iterations
    mov bx, 0                       ; Offset stays at 0
    mov ax, 0x1000                  ; Starting segment
    mov di, 9                       ; Starting LBA
.loop:
    ; Setup DAP with current segment and offset 0
    mov word [dap_offset], 0        ; Always offset 0
    mov word [dap_segment], ax      ; Current segment
    ; ... read 8 sectors ...
    add di, 8                       ; Next LBA
    add ax, 0x200                   ; Advance segment by 8 sectors (4096 bytes = 0x200 * 16)
    loop .loop
```

**Option B: 32-bit addressing in unreal mode**
- Enable unreal mode (load GDT with 4GB limit, return to real mode with cached descriptors)
- Use 32-bit addressing for disk buffer
- More complex but allows loading anywhere in 4GB space

---

## When to Increase Bootloader Capacity

### Scenarios Requiring >64KB Kernel

1. **Static linking LLVM JIT** (not recommended)
   - LLVM ORC JIT libraries: ~300-500KB
   - libLLVMCore, libLLVMExecutionEngine, libLLVMSupport, etc.
   - Would require 512-1024 sector capacity
   - **Better approach**: Keep LLVM as external modules loaded from disk

2. **Embedding large datasets** (e.g., LLM weights)
   - TinyLlama weights: ~40MB
   - Would require ~80,000 sectors
   - **Better approach**: Load from disk using FAT16 or custom format

3. **Multiple optimization levels pre-compiled**
   - Embedding O0/O1/O2/O3 versions of all modules
   - Could reach 100-200KB
   - **Better approach**: Use adaptive JIT to compile on demand

### Recommended Strategy (Current Plan)

✅ **Keep kernel small (<64KB)**
✅ **Load modules from disk via FAT16**
✅ **JIT compile with LLVM at runtime**
✅ **Cache optimized versions to disk**

This approach:
- Keeps bootloader simple and reliable
- Allows unlimited module sizes (only limited by disk space)
- Enables true runtime optimization
- Avoids complex bootloader modifications

---

## LLVM Integration Path Forward

### Phase 1: Minimal LLVM ORC JIT for Bare-Metal
Port `kernel/jit_llvm18.cpp` to bare-metal environment:

1. **C++ Runtime Requirements** (minimal)
   - `__cxa_pure_virtual`: Already implemented ✅
   - `operator new/delete`: Already implemented ✅
   - Exception handling stubs: `__cxa_allocate_exception`, etc. (add minimal stubs)
   - RTTI stubs: `__dynamic_cast` (add stub returning NULL)

2. **LLVM Static Libraries** (link into kernel)
   - libLLVMOrcJIT.a
   - libLLVMExecutionEngine.a
   - libLLVMCore.a
   - libLLVMSupport.a
   - Estimated size: 300-500KB

3. **Integration Point**
   - Modify `adaptive_jit.c` to call LLVM JIT instead of Micro-JIT
   - Replace `micro_jit_compile_fibonacci()` with `llvm_jit_compile(bitcode)`
   - Keep atomic code swapping mechanism ✅

### Phase 2: Disk-Based Module System
1. Store .bc files on FAT16 disk image
2. Load with `bitcode_load_from_disk()` ✅
3. JIT compile with LLVM ORC
4. Execute with profiling
5. Recompile at higher optimization levels when hot

### Phase 3: Persistent Cache
1. Write optimized code back to disk
2. On next boot, load cached O2/O3 versions directly
3. Skip JIT compilation for known-hot functions

---

## Decision Summary

✅ **Keep 128-sector bootloader**
✅ **Focus on runtime JIT optimization**
✅ **Load LLVM modules from disk, not embedded**
⚠️ **Revisit bootloader capacity only if disk-based approach proves insufficient**

**Next task**: Port LLVM ORC JIT to bare-metal C++ environment

---

**Document Status**: CURRENT
**Last Updated**: 2025-10-25
**Author**: Session 13 Analysis
