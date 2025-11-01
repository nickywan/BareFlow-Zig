# QEMU Boot Issues - Reference Guide

**Last Updated**: 2025-10-26 (Post-Session 30)
**Purpose**: Memorize all QEMU boot problems and their solutions
**Architecture**: x86-64 bare-metal kernel with Multiboot2

---

## ⚠️ Known Boot Issues & Solutions

### Issue 1: malloc() Triple Fault ⚠️ ONGOING

**Status**: Deferred to Phase 5 (Session 31 - Paging)

**Symptom**:
```
[Test 2] Memory allocator (256 KB heap):
  About to call malloc(1024)...
  [malloc] init_heap() START
  [malloc] Getting heap pointer...
  [malloc] Setting block size...
  [malloc] Setting free_list...
<< TRIPLE FAULT - QEMU REBOOT >>
```

**Root Cause**:
- Crash point: Writing to `free_list` global variable
- 64-bit long mode requires paging for full memory access
- Global variable writes to .bss fail without page tables

**Investigation Done** (Session 29):
1. ✅ Granular debug logging (pinpointed exact crash line)
2. ✅ Manual BSS zeroing in boot.S (rep stosb)
3. ✅ Tested .data vs .bss placement (no difference)
4. ✅ Reduced heap 1 MB → 256 KB (no difference)
5. ✅ malloc_get_heap_size() works (heap partially accessible)
6. ✅ Identified: Global variable write fails

**Workarounds Tested**:
- ❌ .data section for heap (binary >1 MB, still crashes)
- ❌ Reduced heap size (still crashes)
- ❌ Manual BSS zeroing (still crashes)
- ✅ malloc_get_heap_size() works (proves heap accessible for reads)

**Solution** (Session 31):
- Implement 4-level page tables (PML4 → PDPT → PD → PT)
- Identity map kernel sections (0-4 MB)
- Map heap with write permissions
- Load CR3 with PML4 address

**Files**:
- Investigation: `docs/phase4/MALLOC_INVESTIGATION.md` (450 lines)
- Code: `kernel_lib/memory/malloc_llvm.c` (DEBUG_MALLOC logging)
- Boot: `tests/phase4/qemu_llvm_64/boot.S` (BSS zeroing)

**Commit**: `35f679d` - "fix(phase4): Extensive malloc debugging"

---

### Issue 2: Entry Point Naming Mismatch ✅ FIXED

**Status**: Fixed (Session 28)

**Symptom**:
```
ld.lld: error: undefined symbol: main
>>> referenced by entry.asm
```

**Root Cause**:
- Assembly entry point expects `main()` function
- C++ code used `kernel_main()` naming

**Solution**:
```cpp
// Before (WRONG)
extern "C" void kernel_main() { ... }

// After (CORRECT)
extern "C" void main() { ... }
```

**Why**: Bare-metal entry.asm calls `main` directly, no runtime rename.

**Files**:
- `tests/phase4/qemu_llvm_64/kernel.cpp`

**Lesson**: Bare-metal has no startup code to rename entry points.

---

### Issue 3: Serial Function Name Mismatch ✅ FIXED

**Status**: Fixed (Session 29)

**Symptom**:
```
ld.lld: error: undefined symbol: serial_putc
>>> referenced by malloc_llvm.c
```

**Root Cause**:
- Debug code called `serial_putc()`
- Actual function is `serial_putchar()`

**Solution**:
```c
// Before (WRONG)
extern void serial_putc(char c);

// After (CORRECT)
extern void serial_putchar(char c);
```

**Files**:
- `kernel_lib/memory/malloc_llvm.c` (DEBUG_MALLOC)

**Lesson**: Check kernel_lib API before using functions.

---

### Issue 4: BSS Not Zeroed ✅ FIXED

**Status**: Fixed (Session 29)

**Symptom**:
- Global variables contain garbage
- Unpredictable behavior from uninitialized statics

**Root Cause**:
- Multiboot2 does NOT zero .bss automatically
- Need manual initialization in boot.S

**Solution**:
```asm
# boot.S
_start:
    mov $stack_top, %rsp
    cld

    # Zero .bss section (CRITICAL!)
    movabs $_bss_start, %rdi
    movabs $_bss_end, %rcx
    sub %rdi, %rcx          # size = end - start
    xor %eax, %eax          # value = 0
    rep stosb               # memset(bss, 0, size)

    call kernel_main
```

**Also Required** (linker.ld):
```ld
.bss ALIGN(4K) : {
    _bss_start = .;
    *(COMMON)
    *(.bss*)
    _bss_end = .;
}
```

**Files**:
- `tests/phase4/qemu_llvm_64/boot.S`
- `tests/phase4/qemu_llvm_64/linker.ld`

**Lesson**: ALWAYS zero BSS manually in bare-metal boot.

**Commit**: `35f679d` - BSS zeroing implementation

---

### Issue 5: Assembly Syntax Errors ✅ FIXED

**Status**: Fixed (Session 29)

**Symptom**:
```
boot.S:38: error: invalid instruction mnemonic 'extern'
```

**Root Cause**:
- Used C-style `extern _bss_start` in assembly
- Assembly doesn't have `extern` keyword

**Solution**:
```asm
# Before (WRONG)
extern _bss_start
extern _bss_end
mov $_bss_start, %rdi

# After (CORRECT)
movabs $_bss_start, %rdi
movabs $_bss_end, %rcx
```

**Note**: `movabs` required for 64-bit absolute addresses.

**Files**:
- `tests/phase4/qemu_llvm_64/boot.S`

**Lesson**: Use `movabs` for 64-bit addresses, not `mov`.

---

### Issue 6: Red Zone Corruption (64-bit) ⚠️ POTENTIAL

**Status**: Mitigated with `-mno-red-zone`

**Problem**:
- x86-64 ABI allows "red zone" (128 bytes below RSP)
- Interrupts can corrupt red zone data
- Causes random crashes in kernel code

**Solution**:
```make
# kernel_lib/Makefile.llvm
CFLAGS += -mno-red-zone    # CRITICAL for 64-bit kernel!
```

**Why**: Kernel has no user/kernel privilege separation, interrupts can happen anytime.

**Files**:
- `kernel_lib/Makefile.llvm`

**Lesson**: ALWAYS use `-mno-red-zone` for 64-bit kernels.

**Reference**: Intel SDM Volume 1, Section 6.2.2

---

### Issue 7: Code Model for Kernel ✅ FIXED

**Status**: Fixed with `-mcmodel=kernel`

**Problem**:
- Default code model assumes low addresses
- Kernel may be loaded at higher addresses
- Causes relocation errors

**Solution**:
```make
# kernel_lib/Makefile.llvm
CFLAGS += -mcmodel=kernel   # Higher-half addressing
```

**Why**: Kernel code model allows for addresses above 2GB.

**Files**:
- `kernel_lib/Makefile.llvm`

**Lesson**: Use `-mcmodel=kernel` for 64-bit kernels.

---

## 🔧 Critical Build Flags (64-bit)

### Compiler Flags (MUST USE)

```make
# kernel_lib/Makefile.llvm

CC = clang-18
CXX = clang++-18
AR = llvm-ar-18

# C flags for bare-metal 64-bit
CFLAGS = -ffreestanding \
         -nostdlib \
         -fno-pie \
         -O2 \
         -Wall -Wextra \
         -fno-stack-protector \
         -mno-red-zone \          # CRITICAL! Prevent red zone corruption
         -mcmodel=kernel \        # CRITICAL! Higher-half addressing
         -DBARE_METAL \
         -DHEAP_SIZE_SMALL \      # 256 KB heap for QEMU
         -DDEBUG_MALLOC           # Granular malloc logging

# C++ flags for cpp_runtime
CXXFLAGS = -ffreestanding \
           -nostdlib \
           -fno-pie \
           -O2 \
           -Wall -Wextra \
           -fno-exceptions \      # CRITICAL! No C++ exceptions
           -fno-rtti \            # CRITICAL! No RTTI
           -fno-use-cxa-atexit \
           -fno-stack-protector \
           -mno-red-zone \        # CRITICAL! Same as C
           -mcmodel=kernel        # CRITICAL! Same as C
```

### Why Each Flag is Critical

| Flag | Purpose | Consequence if Missing |
|------|---------|------------------------|
| `-mno-red-zone` | Disable 128-byte red zone below RSP | Interrupt corruption → random crashes |
| `-mcmodel=kernel` | Allow higher-half addresses | Relocation errors at link time |
| `-fno-exceptions` | No C++ exception handling | Undefined symbols (__cxa_throw) |
| `-fno-rtti` | No runtime type info | Undefined symbols (typeinfo) |
| `-fno-stack-protector` | No stack canary | Undefined symbols (__stack_chk_fail) |
| `-ffreestanding` | No hosted environment | Wrong assumptions about libc |
| `-nostdlib` | No standard library linking | Undefined symbols (malloc, printf) |

---

## 🚀 QEMU Testing Workflow

### Build & Test Commands

```bash
# 1. Build kernel_lib_llvm.a (64-bit)
cd kernel_lib
make -f Makefile.llvm clean
make -f Makefile.llvm
# → kernel_lib_llvm.a (29 KB)

# 2. Build QEMU kernel
cd ../tests/phase4/qemu_llvm_64
make clean
make
# → kernel.elf (13 KB + 1 MB BSS)

# 3. Build bootable ISO
make iso
# → bareflow.iso (GRUB + Multiboot2)

# 4. Test in QEMU
make run
# → qemu-system-x86_64 -cdrom bareflow.iso -serial stdio

# 5. Debug with QEMU monitor
qemu-system-x86_64 -cdrom bareflow.iso -serial stdio -monitor stdio -d int,cpu_reset
```

### QEMU Debug Commands

```bash
# Monitor page tables (future - after paging implemented)
(qemu) info mem

# Show registers
(qemu) info registers

# Show interrupts
(qemu) info irq

# Show PIC state
(qemu) info pic

# Dump memory
(qemu) x/64xb 0x100000   # Dump 64 bytes at kernel start
```

---

## 📁 QEMU Test Structure

```
tests/phase4/qemu_llvm_64/
├── boot.S              # Multiboot2 entry (64-bit)
│   ├── Multiboot2 header
│   ├── BSS zeroing (rep stosb)
│   └── Call kernel_main
├── kernel.cpp          # 64-bit kernel main
│   ├── Serial I/O tests
│   ├── malloc tests (TRIPLE FAULT!)
│   └── System halt
├── linker.ld           # 64-bit linker script
│   ├── 1 MB load address
│   ├── .text, .rodata, .data, .bss sections
│   └── _bss_start/_bss_end symbols
├── Makefile            # Build system
│   ├── Compile kernel.cpp + boot.S
│   ├── Link with kernel_lib_llvm.a
│   ├── Create GRUB ISO
│   └── QEMU run command
└── isodir/
    └── boot/grub/grub.cfg   # GRUB config
```

---

## 🔍 Debugging Checklist

When QEMU boot fails:

### 1. Check Serial Output
```bash
make run | tee boot.log
# Check for:
# - Serial initialization message
# - Debug logging output
# - Last message before crash
```

### 2. Check BSS Zeroing
```bash
# Verify BSS symbols in linker script
grep _bss linker.ld

# Verify BSS zeroing in boot.S
grep -A 5 "Zero .bss" boot.S
```

### 3. Check Build Flags
```bash
# Verify critical flags present
grep "mno-red-zone" ../../../kernel_lib/Makefile.llvm
grep "mcmodel=kernel" ../../../kernel_lib/Makefile.llvm
```

### 4. Check Symbol Resolution
```bash
# List undefined symbols
nm kernel.elf | grep " U "

# Should be empty! All symbols must be resolved.
```

### 5. Check Kernel Size
```bash
size kernel.elf
# Verify:
# - text: ~12-15 KB (code)
# - data: <1 KB (initialized data)
# - bss: ~1 MB (uninitialized, heap)
```

### 6. Check GRUB ISO
```bash
# Verify kernel is in ISO
isoinfo -l -i bareflow.iso | grep kernel
# Should show: /boot/kernel.elf
```

---

## 📚 Reference Documents

### Investigation Reports
- `docs/phase4/MALLOC_INVESTIGATION.md` - malloc triple fault (450 lines)
- `docs/phase4/SESSION_28_SUMMARY.md` - Tiered compilation validation
- `docs/phase4/SESSION_30_SUMMARY.md` - Phase 4 complete review

### Architecture Decisions
- `docs/phase4/ARCH_DECISION_64BIT.md` - 32-bit → 64-bit migration

### Code References
- `kernel_lib/Makefile.llvm` - Build system with all flags
- `tests/phase4/qemu_llvm_64/boot.S` - Multiboot2 entry + BSS zeroing
- `tests/phase4/qemu_llvm_64/kernel.cpp` - 64-bit kernel example

---

## 🎯 Session 31 Prerequisites

Before starting paging implementation, verify:

1. ✅ kernel_lib_llvm.a builds (29 KB)
2. ✅ QEMU kernel boots (serial output working)
3. ✅ BSS zeroing implemented (manual rep stosb)
4. ✅ All build flags correct (-mno-red-zone, -mcmodel=kernel)
5. ✅ malloc investigation documented
6. ⚠️ malloc triple fault understood (needs paging)

**Ready to implement**: 4-level page tables (Session 31)

---

## 💡 Key Lessons

1. **QEMU is the truth** - Test in real x86-64 environment, not userspace simulation
2. **Serial debugging is critical** - COM1 output is only way to see what's happening
3. **BSS must be zeroed manually** - Multiboot2 doesn't do this
4. **64-bit flags are non-negotiable** - `-mno-red-zone` and `-mcmodel=kernel` required
5. **Investigate thoroughly** - 7 approaches tried for malloc before deferring
6. **Document everything** - Future sessions will thank you

---

**Last Updated**: 2025-10-26 (Post-Session 30)
**Next**: Session 31 - Paging Implementation (CRITICAL)
**Status**: All known issues documented ✅
