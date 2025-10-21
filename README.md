# 🔥 Fluid OS - Self-Optimizing Kernel

**A unikernel with LLVM JIT runtime that self-optimizes at runtime.**

## Quick Start

### Phase 1 (User-Space JIT) - DONE ✅
```bash
# Build module
clang-18 -c -emit-llvm -O2 -ffreestanding -nostdlib \
    kernel/stdlib.c -o kernel/stdlib.bc
clang-18 -c -emit-llvm -O2 -ffreestanding -nostdlib \
    app/test.c -o app/test.bc
llvm-link-14 kernel/stdlib.bc app/test.bc -o fluid_module.bc

# Test JIT
cd jit_test
./jit_profiler ../fluid_module.bc
```

### Phase 2 (Kernel Bare Metal) - IN PROGRESS 🚧
```bash
make
make run
```

## Architecture

```
Application (LLVM IR)
       ↕
Kernel Fluid (Ring 0)
  ├── LLVM ORC JIT
  ├── Profiler Runtime
  └── Re-JIT Optimizer
```

**Innovation**: Kernel that JITs itself to adapt to the workload.

## Boot Process - Two Stage Bootloader

The system uses a two-stage bootloader to load the kernel in 32-bit protected mode.

### 📋 Disk Layout

```
┌────────────────────────────────────┐
│ Sector 0 (512 bytes)               │ ← Stage 1 Bootloader
├────────────────────────────────────┤
│ Sectors 1-8 (4 KB)                 │ ← Stage 2 Bootloader
├────────────────────────────────────┤
│ Sectors 9+ (variable)              │ ← Kernel Binary
└────────────────────────────────────┘
```

### 🚀 Stage 1 - Initial Bootloader (boot/stage1.asm)

**Size**: 512 bytes (1 sector)
**Load address**: 0x7C00 (loaded by BIOS)
**Responsibilities**:

1. **Basic initialization**
   - Configure segments (DS, ES, SS = 0x0000)
   - Configure stack (SP = 0x7C00, descending stack)
   - Save boot drive number (DL)

2. **Load Stage 2**
   - Load 8 sectors (4 KB) from disk
   - Sectors 2-9 → memory address 0x7E00
   - Uses INT 0x13 (BIOS disk services) in CHS mode
   - 3 retry attempts on error

3. **Transfer control**
   - Pass disk number to Stage 2 (via DL)
   - Jump to Stage 2 at address 0x7E00

**Limitations**:
- Real mode 16-bit only
- No LBA support (CHS only for simplicity)
- Fixed size of 512 bytes (BIOS constraint)

### 🔧 Stage 2 - Extended Bootloader (boot/stage2.asm)

**Size**: 4 KB (8 sectors)
**Load address**: 0x7E00
**Responsibilities**:

#### 1. **Detect LBA support**
```
INT 0x13, AH=0x41 (Check Extensions Present)
→ Allows loading kernel with LBA (modern) or CHS (fallback)
```

#### 2. **Enable A20 line**
The A20 line enables access to memory beyond 1 MB. Three methods attempted in order:

| Method | Description | Advantage |
|---------|-------------|----------|
| **BIOS** | INT 0x15, AX=0x2401 | Simple and portable |
| **Keyboard Controller** | Via ports 0x64/0x60 | Compatible with older PCs |
| **Fast A20** | Port 0x92 | Fast on recent PCs |

Verification with memory test at 0x7E00 vs 0xFFFF:0x7E10.

#### 3. **Load kernel**
```
Sector 9+ → memory address 0x1000
Size: 15 sectors (~7.5 KB)
Method: LBA (preferred) or CHS (fallback)
Retry: 3 attempts maximum
```

#### 4. **Signature verification**
```
Verify that first 4 bytes of kernel = "FLUD" (0x464C5544)
Prevents execution of corrupted data
```

#### 5. **GDT Configuration** (Global Descriptor Table)
```
┌─────────────┬────────────────┬──────────────────┐
│ NULL (0x00) │ CODE (0x08)    │ DATA (0x10)      │
└─────────────┴────────────────┴──────────────────┘
       ↓              ↓                  ↓
   Required      Ring 0, 4GB       Ring 0, 4GB
               Executable         Writable
```

#### 6. **Protected mode transition**
```assembly
cli                    ; Disable interrupts
lgdt [gdt_descriptor]  ; Load GDT
mov cr0, 0x1          ; Enable PE (Protection Enable) bit
jmp CODE_SEG:protected_mode_start  ; Far jump to flush pipeline
```

#### 7. **Protected mode initialization (32-bit)**
```
- Configure all segments (DS, SS, ES, FS, GS) → 0x10 (DATA_SEG)
- Configure stack at 0x90000 (EBP/ESP)
- Jump to kernel at 0x1000
```

### 🎯 Kernel Entry Point (kernel/entry.asm)

**Signature**: `"FLUD"` (0x464C5544) - first 4 bytes
**Address**: 0x1000
**Format**: ELF 32-bit flat binary

The kernel receives control in 32-bit protected mode with:
- CS = 0x08 (code segment)
- DS/SS/ES/FS/GS = 0x10 (data segment)
- ESP = 0x90000 (configured stack)
- Interrupts disabled (CLI)

Then `kernel_main()` takes over (kernel/kernel.c).

### 🔍 Boot Sequence Summary

```
┌─────────────┐
│    BIOS     │ Load Sector 0 → 0x7C00
└──────┬──────┘
       ↓
┌─────────────┐
│   Stage 1   │ Load Sectors 1-8 → 0x7E00
│  (0x7C00)   │ Jump → 0x7E00
└──────┬──────┘
       ↓
┌─────────────┐
│   Stage 2   │ 1. Detect LBA
│  (0x7E00)   │ 2. Enable A20
│             │ 3. Load Kernel → 0x1000
│             │ 4. Verify "FLUD" signature
│             │ 5. Configure GDT
│             │ 6. Protected mode
│             │ 7. Jump → 0x1000
└──────┬──────┘
       ↓
┌─────────────┐
│   Kernel    │ kernel_main() in Ring 0
│  (0x1000)   │ 32-bit protected mode
└─────────────┘
```

### ⚠️ Error Handling

Stage 2 uses hexadecimal error codes:

| Code | Error | Probable Cause |
|------|--------|----------------|
| `0xD1` | DISK_READ | Disk read failure |
| `0xD2` | DISK_VERIFY | Incorrect sector count |
| `0xA2` | A20_FAILED | A20 line cannot be enabled |
| `0x51` | BAD_SIGNATURE | Invalid kernel signature |
| `0xE0` | NO_KERNEL | Kernel not found |

On fatal error, the system displays `FATAL ERROR: 0xXX` and halts (CLI/HLT).

## Build System

The Makefile orchestrates the complete build:

```bash
make         # Build everything
make run     # Build + launch QEMU
make debug   # Build + QEMU with CPU logs
make clean   # Clean artifacts
make rebuild # Clean + build
```

**Build pipeline**:
1. Stage 1 (NASM) → `build/stage1.bin` (exactly 512 bytes)
2. Stage 2 (NASM) → `build/stage2.bin` (exactly 4096 bytes)
3. Kernel ASM (NASM) + C (GCC) → `build/kernel.bin`
4. Assembly with `dd` → `fluid.img` (1.44 MB floppy)

## Status

- ✅ Phase 1: POC User-Space (VALIDATED)
- 🚧 Phase 2: Kernel Bare Metal (IN PROGRESS)
  - ✅ Two-stage bootloader
  - ✅ Protected mode
  - ✅ VGA text mode
  - ✅ Memory allocator
  - ⏳ LLVM JIT integration
- ⏳ Phase 3: llama.cpp Integration (FUTURE)

## Documentation

See [PROJECT.md](PROJECT.md) for complete documentation.

## License

Educational/Research - Not production ready
