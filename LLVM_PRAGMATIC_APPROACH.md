# LLVM Integration - Pragmatic Approach

**Date**: 2025-10-25
**Context**: After analyzing bitcode format and LLVM IR complexity
**Revision**: Simplified strategy based on reality check

---

## Reality Check: LLVM IR Interpreter is Too Complex

### Original Plan Issues
1. **LLVM bitcode format**: Binary compressed format, requires BitReader library
2. **LLVM IR semantics**: 200+ instructions, complex type system, SSA form
3. **Implementation effort**: 4-6 weeks minimum for basic interpreter
4. **Testing burden**: Need to validate every instruction type
5. **Maintenance**: LLVM IR evolves, parser needs updates

### What We Actually Need
**User priority**: "runtime optimization, not kernel size"

Translation: We need **working code that runs and gets faster**, not perfect JIT.

---

## NEW RECOMMENDED APPROACH: Expand Bootloader + Module-Based LLVM

### Strategy Overview

Instead of fighting kernel size constraints, **embrace disk-based module loading**:

1. **Keep kernel minimal** (current 57KB)
2. **Expand bootloader to 512 sectors** (256KB capacity)
3. **Load LLVM JIT as a module** from disk at runtime
4. **Progressive loading**: Boot fast, load JIT on demand

### Why This Works

✅ **Aligns with user priority**: Runtime optimization focus
✅ **Uses existing infrastructure**: FAT16 disk loading ✅, module system ✅
✅ **Leverages real LLVM**: Full optimization power, tested compiler
✅ **Simpler implementation**: Use LLVM APIs, not reimplement them
✅ **Extensible**: Can load other compilers (tinycc, etc.) later

---

## Implementation Plan

### Phase 1: Fix Bootloader to Support 512 Sectors (1 day)

**Problem**: Previous 256-sector attempt failed due to segment boundary issues

**Solution**: Use proper segment advancement

```asm
load_kernel_lba:
    mov cx, 64                      ; 64 iterations (512 sectors = 256KB)
    mov ax, 0x1000                  ; Starting segment
    mov di, 9                       ; Starting LBA
    xor bx, bx                      ; Offset always 0

.loop:
    push cx

    ; Setup DAP
    mov byte [dap_size], 0x10
    mov word [dap_sectors], 8
    mov word [dap_offset], 0        ; Always offset 0 within segment
    mov word [dap_segment], ax
    mov word [dap_lba_low], di
    mov word [dap_lba_low+2], 0
    mov dword [dap_lba_high], 0

    ; Read
    mov si, dap_size
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .error

    ; Advance: 8 sectors = 4096 bytes = 0x1000 bytes
    ; In segment:offset form, 0x1000 bytes = advance segment by 0x100
    add di, 8                       ; Next LBA
    add ax, 0x100                   ; Next segment (4096/16 = 256 = 0x100)

    pop cx
    loop .loop

.success:
    clc
    ret

.error:
    pop cx
    stc
    ret
```

**Testing**:
1. Build with 512-sector bootloader
2. Pad kernel to 200KB with dummy data
3. Verify boot succeeds and kernel loads correctly

### Phase 2: Create LLVM Module Loader (2-3 days)

**Goal**: Load LLVM static libraries from disk as relocatable modules

#### 2.1: ELF Parser for Bare-Metal (kernel/elf_loader.{h,c})

Minimal ELF loader without libc:

```c
typedef struct {
    void* base_addr;           // Where ELF is loaded in memory
    uint32_t entry_point;      // Main entry (_start or specified)
    uint32_t num_symbols;
    elf_symbol_t* symbols;     // Exported symbols
} elf_module_t;

// Load ELF from memory buffer
int elf_load(const uint8_t* elf_data, size_t size, elf_module_t** out);

// Resolve symbol by name
void* elf_get_symbol(elf_module_t* mod, const char* name);
```

**Supported**:
- 32-bit ELF only (x86)
- Static linking (no PLT/GOT relocation)
- Text + data + bss sections
- Symbol table parsing

**Not supported**:
- Shared libraries (.so)
- Dynamic linking
- Thread-local storage
- C++ exceptions (already disabled)

#### 2.2: LLVM Module Stub (kernel/llvm_module.{h,c})

Wrapper around LLVM ORC JIT loaded as module:

```c
typedef struct llvm_jit_module llvm_jit_module_t;

// Load LLVM JIT from disk
// This reads "llvm_jit.elf" from FAT16 and loads it using elf_loader
int llvm_module_load(fat16_fs_t* fs, llvm_jit_module_t** out);

// Initialize LLVM JIT (calls into loaded module)
int llvm_module_init(llvm_jit_module_t* mod);

// Compile bitcode to native code
void* llvm_module_compile(llvm_jit_module_t* mod, const uint8_t* bitcode, size_t size);

// Execute compiled function
int llvm_module_execute(llvm_jit_module_t* mod, void* func_ptr, uint32_t* args, uint32_t num_args);
```

### Phase 3: Build LLVM as Freestanding Static Binary (2 days)

**Goal**: Compile LLVM ORC JIT to run on bare-metal

#### 3.1: LLVM Minimal Configuration

Create `llvm_jit_minimal.cpp`:

```cpp
// Minimal LLVM JIT for bare-metal
// Compiled to freestanding ELF

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;
using namespace llvm::orc;

// Bare-metal allocator
extern "C" void* malloc(size_t size);
extern "C" void free(void* ptr);

struct BareMetalJIT {
    std::unique_ptr<LLJIT> jit;
    std::unique_ptr<LLVMContext> context;

    BareMetalJIT() {
        // Initialize target
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();

        context = std::make_unique<LLVMContext>();

        auto jit_expected = LLJITBuilder().create();
        if (jit_expected) {
            jit = std::move(*jit_expected);
        }
    }

    void* compile(const uint8_t* bitcode, size_t size) {
        auto buffer = MemoryBuffer::getMemBuffer(
            StringRef((const char*)bitcode, size), "", false);

        // Parse bitcode
        auto module_expected = parseBitcodeFile(*buffer, *context);
        if (!module_expected) return nullptr;

        // Add to JIT
        auto tsm = ThreadSafeModule(std::move(*module_expected),
                                     std::make_unique<LLVMContext>());
        if (auto err = jit->addIRModule(std::move(tsm))) {
            return nullptr;
        }

        // Lookup main function (assume "entry")
        auto symbol_expected = jit->lookup("entry");
        if (!symbol_expected) return nullptr;

        return (void*)symbol_expected->getValue();
    }
};

// C API for kernel
extern "C" {
    BareMetalJIT* llvm_jit_create() {
        return new BareMetalJIT();
    }

    void* llvm_jit_compile(BareMetalJIT* jit, const uint8_t* bc, size_t size) {
        return jit->compile(bc, size);
    }
}
```

#### 3.2: Compilation

```bash
# Compile LLVM JIT minimal to freestanding ELF
clang++-18 -m32 -ffreestanding -nostdlib -fno-pie \
    -fno-exceptions -fno-rtti -fno-threadsafe-statics \
    -O2 -march=i686 \
    -I/usr/lib/llvm-18/include \
    -c llvm_jit_minimal.cpp -o llvm_jit_minimal.o

# Link with LLVM static libraries
ld -m elf_i386 -r llvm_jit_minimal.o \
    /usr/lib/llvm-18/lib/libLLVMOrcJIT.a \
    /usr/lib/llvm-18/lib/libLLVMCore.a \
    /usr/lib/llvm-18/lib/libLLVMExecutionEngine.a \
    /usr/lib/llvm-18/lib/libLLVMSupport.a \
    /usr/lib/llvm-18/lib/libLLVMMC.a \
    /usr/lib/llvm-18/lib/libLLVMObject.a \
    /usr/lib/llvm-18/lib/libLLVMX86CodeGen.a \
    /usr/lib/llvm-18/lib/libLLVMX86Desc.a \
    /usr/lib/llvm-18/lib/libLLVMX86Info.a \
    -o llvm_jit.elf

# Check size
ls -lh llvm_jit.elf
# Expected: 5-10MB (large but loaded from disk, not in kernel)
```

### Phase 4: Integrate with Adaptive JIT (2 days)

Modify `kernel/adaptive_jit.c`:

```c
typedef struct {
    char name[64];
    bitcode_module_t* bitcode;     // Bitcode from disk
    void* code_ptr;                 // JIT compiled code
    llvm_jit_module_t* llvm;        // LLVM JIT module
    adaptive_jit_state_t state;     // O0/O1/O2/O3
    uint32_t call_count;
    bool is_llvm_compiled;
} adaptive_llvm_module_t;

// Load module from disk and JIT compile
int adaptive_llvm_load(fat16_fs_t* fs, const char* filename,
                       adaptive_llvm_module_t** out) {
    // 1. Load bitcode from FAT16
    bitcode_module_t* bc;
    if (bitcode_load_from_disk(fs, filename, &bc) != 0) {
        return -1;
    }

    // 2. Load LLVM JIT if not loaded
    static llvm_jit_module_t* llvm_jit = NULL;
    if (!llvm_jit) {
        llvm_module_load(fs, &llvm_jit);
        llvm_module_init(llvm_jit);
    }

    // 3. Compile bitcode
    void* func_ptr = llvm_module_compile(llvm_jit,
                                          bc->bitcode_data,
                                          bc->header.bitcode_size);

    // 4. Create adaptive module
    adaptive_llvm_module_t* mod = malloc(sizeof(*mod));
    mod->bitcode = bc;
    mod->code_ptr = func_ptr;
    mod->llvm = llvm_jit;
    mod->state = ADAPTIVE_O0;
    mod->call_count = 0;
    mod->is_llvm_compiled = true;

    *out = mod;
    return 0;
}

// Execute with profiling and adaptive recompilation
uint32_t adaptive_llvm_execute(adaptive_llvm_module_t* mod,
                               uint32_t* args, uint32_t num_args) {
    mod->call_count++;

    // Check for optimization thresholds
    if (mod->call_count == 100 && mod->state == ADAPTIVE_O0) {
        // Recompile with O1
        mod->state = ADAPTIVE_O1;
        // TODO: Tell LLVM to recompile with -O1
    } else if (mod->call_count == 1000 && mod->state == ADAPTIVE_O1) {
        // Recompile with O2
        mod->state = ADAPTIVE_O2;
        // TODO: Tell LLVM to recompile with -O2
    }

    // Execute
    return llvm_module_execute(mod->llvm, mod->code_ptr, args, num_args);
}
```

### Phase 5: End-to-End Demo (1 day)

```c
void demo_llvm_jit_adaptive(fat16_fs_t* fs) {
    terminal_writestring("=== LLVM Adaptive JIT Demo ===\n");

    // Load module from disk
    adaptive_llvm_module_t* fib_mod;
    adaptive_llvm_load(fs, "fibonacci.bc", &fib_mod);

    // Execute 2000 times with profiling
    for (int i = 0; i < 2000; i++) {
        uint32_t args[1] = {5};
        uint64_t start = rdtsc();
        uint32_t result = adaptive_llvm_execute(fib_mod, args, 1);
        uint64_t end = rdtsc();

        if (i % 100 == 0) {
            terminal_writestring("Iteration ");
            print_dec(i);
            terminal_writestring(": fib(5) = ");
            print_dec(result);
            terminal_writestring(" (");
            print_dec(end - start);
            terminal_writestring(" cycles)\n");
        }
    }

    terminal_writestring("Demo complete!\n");
}
```

---

## Size Analysis

### Kernel Binary
- Current: 57KB
- With ELF loader + LLVM stub: ~65KB
- With 512-sector bootloader: **fits easily** ✅

### LLVM Module (on disk)
- llvm_jit.elf: 5-10MB
- Loaded once at runtime into high memory (0x1000000+)
- Does not count against kernel size

### Disk Image
- Boot: 1 sector (512B)
- Stage 2: 8 sectors (4KB)
- Kernel: ~130 sectors (~65KB)
- Modules: Rest of 1.44MB floppy (~1.3MB available)
- LLVM JIT: ~20,000 sectors (~10MB) - **would need larger disk image**

**Solution**: Use 10MB disk image instead of 1.44MB floppy

---

## Advantages of This Approach

✅ **Full LLVM optimization**: -O0/-O1/-O2/-O3 real compiler optimizations
✅ **Tested and maintained**: Use upstream LLVM, not custom interpreter
✅ **Extensible**: Can add other tools (assembler, linker, debugger)
✅ **Smaller kernel**: Keep kernel focused, tools on disk
✅ **Fast boot**: Kernel loads in <1 second, LLVM loads on demand
✅ **User priority**: "Runtime optimization" ← THIS DELIVERS IT

## Disadvantages

⚠️ **Initial load time**: Loading 10MB from disk takes ~5 seconds
⚠️ **Memory usage**: LLVM JIT uses ~20MB RAM (need to manage heap)
⚠️ **Complex integration**: ELF loading, symbol resolution, calling convention

## Risk Mitigation

1. **ELF loading**: Use existing libraries (parse_elf.c from Linux kernel)
2. **Symbol resolution**: Only need ~10 kernel symbols (malloc, memcpy, etc.)
3. **Memory management**: Allocate fixed 32MB region for LLVM at boot
4. **Testing**: Build incrementally, test each phase

---

## Timeline

| Phase | Duration | Cumulative |
|-------|----------|------------|
| 1. Fix bootloader (512 sectors) | 1 day | 1 day |
| 2. ELF loader | 2 days | 3 days |
| 3. LLVM minimal build | 2 days | 5 days |
| 4. Adaptive JIT integration | 2 days | 7 days |
| 5. End-to-end demo | 1 day | 8 days |

**Total: 8 days (~2 weeks)**

Compare to IR interpreter: 4-6 weeks + ongoing maintenance

---

## Recommendation

✅ **ADOPT THIS APPROACH**

**Rationale**:
1. Faster to implement (2 weeks vs 6 weeks)
2. Better performance (real LLVM vs interpreter)
3. More maintainable (use LLVM APIs)
4. Aligns with user directive (runtime optimization focus)
5. Extensible (can add TCC, GCC plugins, etc. later)

**Next Steps**:
1. Fix bootloader to 512 sectors (Phase 1)
2. Test with padded kernel
3. Implement ELF loader (Phase 2)
4. Build LLVM minimal (Phase 3)
5. Integrate and demo (Phases 4-5)

---

**Status**: RECOMMENDED APPROACH
**Approval**: Awaiting user confirmation
**Alternative**: IR interpreter (if bootloader expansion fails again)
