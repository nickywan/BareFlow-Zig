# CLAUDE CONTEXT - Fluid OS Development

**Date**: 2025-10-25
**Branch**: `claude/merge-interface-runtime-011CUMDiW4omhPaJemQSVuoR`
**Last Commit**: `2f58519` - Update roadmap: mark Phase 1.2 profiling export as completed

---

## 🎯 Current Project State

**Fluid OS** is a bare-metal unikernel with LLVM-based runtime optimization. The system boots from a two-stage bootloader into 32-bit protected mode and supports dynamic module loading with cycle-accurate profiling.

### ✅ What Works Now
- Two-stage bootloader (Stage 1: 512 bytes, Stage 2: 4KB)
- 32-bit protected mode kernel at 0x10000 (64KB)
- VGA text mode (80×25, 16 colors)
- PS/2 keyboard input (interactive mode)
- AOT module system with 4 embedded modules
- Cycle-accurate profiling using `rdtsc`
- **Serial port driver (COM1, 115200 baud)**
- **JSON profiling export via serial port** ✨ NEW
- C++ runtime support (new/delete, global constructors)
- Custom JIT allocator (CODE/DATA/METADATA pools)
- Interactive vs automated build modes

### 📊 Kernel Stats
- **Size**: 37,392 bytes (73 sectors)
- **Bootloader capacity**: 80 sectors (40KB)
- **Memory layout**:
  - Kernel: 0x10000 (64KB)
  - Stack: 0x90000 (grows down)
  - Heap: 0x100000 (1MB, 64KB allocated)

---

## 🔥 Recent Session Work (2025-10-25)

### Problem Solved: Serial JSON Export Corruption

**Issue**: JSON export was corrupted with null bytes replacing most characters.

**Root Cause**: Bootloader loaded only 64 sectors (32KB) but kernel grew to 37KB after adding profiling export. The `.rodata` section wasn't being loaded!

**Fix**:
- Increased `KERNEL_SECTORS` from 64 to 80 in `boot/stage2.asm`
- Updated `load_kernel_lba()` to use 10 iterations (10 × 8 sectors)

**Commits**:
- `bdf5d2a` - Fix bootloader: increase KERNEL_SECTORS from 64 to 80
- `2f58519` - Update roadmap: mark Phase 1.2 profiling export as completed

**Result**: ✅ Clean JSON export validated with `python3 -m json.tool`

---

## 📋 Phase 1.2: Profile-Guided Optimization System

### Architecture Decision: AOT + Offline Recompilation

**Why not bare-metal JIT?**
- Full LLVM OrcJIT requires ~500KB + libc++ (3-6 months work)
- Host has full LLVM toolchain available
- Persistent optimization cache across reboots

**Workflow**:
1. Kernel profiles modules with `rdtsc` (call count, min/max/total cycles)
2. Export profiling data via serial port (JSON format)
3. Host parses JSON and identifies hot modules
4. Host recompiles hot modules with `-O2/-O3 + PGO`
5. Optimized modules stored in persistent cache
6. Kernel loads optimized versions from cache at next boot

### ✅ Completed Tasks (Phase 1.2)
- [x] JSON format for profiling statistics
- [x] Serial port driver (COM1) with loopback test
- [x] Export per-module: calls, total_cycles, min_cycles, max_cycles, code_address
- [x] Automated export at boot (no VGA interference)
- [x] Preprocessor directives for interactive mode (#ifdef INTERACTIVE_MODE)

### 🔜 Next Tasks (Phase 1.2)
- [ ] Design optimized module cache structure on disk
- [ ] Implement cache loader at boot with signature verification
- [ ] Create offline recompilation pipeline (`tools/pgo_recompile.py`)
- [ ] Add `matrix_mul` benchmark module for validation
- [ ] Test end-to-end: profile → recompile → reload cycle

---

## 🏗️ Architecture Overview

### Boot Process
```
BIOS → Stage 1 (0x7C00, 512 bytes)
     → Stage 2 (0x7E00, 4KB)
       - Enable A20 line
       - Load kernel (80 sectors, LBA mode)
       - Verify "FLUD" signature
       - Setup GDT (flat segments)
       - Enter protected mode
     → Kernel Entry (0x10000)
       - Setup stack at 0x90000
       - Initialize VGA, keyboard, C++ runtime
       - Initialize module manager
       - Load embedded modules
       - Run module tests (4 tests, 13 calls total)
       - Export profiling data via serial
       - Halt
```

### Module System (AOT)
- **Location**: `kernel/module_loader.{h,c}`, `kernel/embedded_modules.h`
- **Modules**: Pre-compiled with `clang-18 -m32 -ffreestanding -O2`
- **Embedded modules**: fibonacci, sum, compute, primes
- **Execution**: `module_execute()` wraps calls with rdtsc profiling

### Profiling Export System
- **Driver**: `kernel/profiling_export.{h,c}`
- **Port**: COM1 (0x3F8), 115200 baud, 8N1
- **Format**: JSON with format_version, timestamp, modules array
- **Output**: Serial port only (no VGA during export)

---

## 📁 Key Files

### Bootloader
- `boot/stage1.asm` - MBR bootloader (512 bytes)
- `boot/stage2.asm` - Extended bootloader (4KB, 80-sector kernel support)

### Kernel Core
- `kernel/entry.asm` - Entry point with "FLUD" signature
- `kernel/kernel.c` - Main kernel, module tests, profiling export integration
- `kernel/linker.ld` - Linker script (kernel at 0x10000)

### Drivers
- `kernel/vga.{h,c}` - VGA text mode (80×25)
- `kernel/keyboard.h` - PS/2 keyboard input
- `kernel/profiling_export.{h,c}` - Serial port driver + JSON export

### Module System
- `kernel/module_loader.{h,c}` - AOT module loader with profiling
- `kernel/embedded_modules.h` - Embedded module declarations
- `modules/*.c` - Module source code
- `modules/*.mod` - Compiled modules (generated)

### Runtime & Memory
- `kernel/stdlib.{h,c}` - Bare-metal C library
- `kernel/jit_allocator.{h,c}` - Three-pool allocator (CODE/DATA/METADATA)
- `kernel/cxx_runtime.cpp` - C++ runtime (new/delete, global ctors)

### Build System
- `Makefile` - Main kernel build (with INTERACTIVE mode support)
- `Makefile.jit` - JIT interface build
- `Makefile.modules` - Module compilation

---

## 🔧 Build Commands

### Standard Build
```bash
make                # Automated mode (default)
make INTERACTIVE=1  # Interactive mode (keyboard pauses)
make clean          # Remove build artifacts
make rebuild        # Clean + build
```

### Run & Debug
```bash
make run            # Build and launch in QEMU with serial output
make debug          # Build and run with CPU debug logs
```

### Profiling Data Capture
```bash
# Capture serial output to file
timeout 10 qemu-system-i386 -drive file=fluid.img,format=raw \
  -serial file:/tmp/profiling.json -display none

# Extract and validate JSON
sed -n '/^{$/,/^}$/p' /tmp/profiling.json | python3 -m json.tool
```

### Module System
```bash
make -f Makefile.modules all          # Compile all modules
make -f Makefile.modules list-modules # List compiled modules
```

---

## 📊 Example Profiling Output

```json
{
  "format_version": "1.0",
  "timestamp_cycles": 1332762869,
  "total_calls": 13,
  "num_modules": 4,
  "modules": [
    {
      "name": "fibonacci",
      "calls": 1,
      "total_cycles": 17671,
      "min_cycles": 17671,
      "max_cycles": 17671,
      "code_address": "0x00010020",
      "code_size": 128,
      "loaded": true
    },
    {
      "name": "compute",
      "calls": 10,
      "total_cycles": 3265519,
      "min_cycles": 258151,
      "max_cycles": 938226,
      "code_address": "0x00010040",
      "code_size": 256,
      "loaded": true
    }
  ]
}
```

**Interpretation**:
- `compute` module called 10 times (average: 326,551 cycles)
- High variance (min: 258K, max: 938K) indicates cold-start overhead
- Good candidate for PGO optimization

---

## 🐛 Known Issues & Solutions

### ✅ SOLVED: Serial JSON Export Corruption
**Problem**: Null bytes in serial output
**Cause**: Bootloader loading only 64 sectors, .rodata not loaded
**Fix**: Increased KERNEL_SECTORS to 80

### ✅ SOLVED: VGA/Serial Output Mixing
**Problem**: JSON corrupted by VGA writes
**Solution**: Serial-only output in `profiling_trigger_export()`

### ⚠️ Active Limitations
- **Module limit**: 16 modules max (MAX_MODULES)
- **Module names**: 32 characters max
- **Kernel size**: 40KB max (80 sectors)
- **No floating point**: Would need `-mno-sse -mno-mmx`

---

## 🔍 Debugging Tips

### Boot Errors
- `0xD1` - Disk read failure
- `0xD2` - Incorrect sector count
- `0xA2` - A20 line cannot be enabled
- `0x51` - Invalid kernel signature
- `0xE0` - Kernel not found

### Verify Kernel Signature
```bash
hexdump -n 4 -e '4/1 "%02x"' build/kernel.bin
# Should output: 44554c46 ("FLUD")
```

### Check Kernel Size
```bash
ls -lh build/kernel.bin
stat -c%s build/kernel.bin  # Linux
stat -f%z build/kernel.bin  # macOS
```

### Serial Port Testing
```bash
# Test serial output
qemu-system-i386 -drive file=fluid.img,format=raw -serial stdio

# Capture to file
qemu-system-i386 -drive file=fluid.img,format=raw -serial file:/tmp/serial.txt

# Raw byte inspection
od -c /tmp/serial.txt | head -50
```

---

## 🎯 Next Steps (Recommended Order)

### Immediate (This Session/Next)
1. **Design module cache format**
   - Cache metadata: module name, hash, optimization level, timestamp
   - Native code storage: raw binary + size
   - Disk location: dedicated sectors or FAT16 file?

2. **Create `tools/pgo_recompile.py`**
   - Parse profiling JSON
   - Identify hot modules (threshold: >100 calls or >1M cycles)
   - Trigger recompilation: `clang-18 -O3 -fprofile-use=...`
   - Generate optimized .mod files

3. **Add cache loader in kernel**
   - `module_cache_load()` at boot
   - Verify cache signature/hash
   - Prefer cached version over embedded version

### Short-term (Phase 1.2 completion)
4. **Add `matrix_mul` benchmark module**
   - Dense 64×64 or 128×128 matrix multiplication
   - Measure baseline vs optimized cycle counts
   - Document speedup (expected: 2-5x)

5. **Test end-to-end PGO workflow**
   - Boot → profile → export → recompile → reload → measure

### Medium-term (Phase 1.3)
6. **Integrate llvm-libc subset**
   - Replace `stdlib.c` with llvm-libc (memcpy, memset, etc.)
   - Add math.h functions for future TinyLlama support

7. **CPU feature detection**
   - Host scanner: `clang -march=native -###`
   - Auto-generate compile flags for target machine

---

## 📚 Documentation References

- `README.md` - High-level overview and quick start
- `ROADMAP.md` - Complete project roadmap (Phases 1-5)
- `CLAUDE.md` - Development guidelines (build commands, architecture)
- `CHANGELOG.md` - Version history

---

## 🔐 Git State

**Current Branch**: `claude/merge-interface-runtime-011CUMDiW4omhPaJemQSVuoR`
**Main Branch**: `main`
**Uncommitted Changes**: None (clean working tree)

**Recent Commits**:
```
2f58519 Update roadmap: mark Phase 1.2 profiling export as completed
bdf5d2a Fix bootloader: increase KERNEL_SECTORS from 64 to 80
e5a1359 Refactor: Use preprocessor directives for interactive mode
d94fbb2 Disable keyboard pauses for automated profiling export
4f97aeb Add profiling data export system with serial port driver
5b25c3b Remove generated fluid.img from version control
```

---

## 💡 Important Design Decisions

### 1. AOT vs JIT
**Decision**: AOT modules + offline recompilation
**Rationale**: Full LLVM JIT in bare-metal is 3-6 months work, host already has toolchain

### 2. Serial vs Network for Profiling
**Decision**: Serial port (COM1)
**Rationale**: Simpler, no network stack needed, reliable for profiling data

### 3. Interactive vs Automated Modes
**Decision**: Preprocessor directives (#ifdef INTERACTIVE_MODE)
**Rationale**: Clean code, compile-time choice, 420 bytes saved in automated mode

### 4. Module Cache Location
**Decision**: TBD (pending discussion)
**Options**:
- Dedicated disk sectors (simple, no filesystem)
- FAT16 file (more flexible, requires filesystem)

---

## 🧪 Testing Checklist

### Before Each Commit
- [ ] `make clean && make` succeeds
- [ ] Kernel boots without errors
- [ ] All 4 module tests pass
- [ ] Serial JSON export is valid (test with `python3 -m json.tool`)

### For Bootloader Changes
- [ ] Verify kernel signature with `hexdump`
- [ ] Test with both LBA and CHS fallback
- [ ] Check A20 line detection

### For Module System Changes
- [ ] Recompile modules: `make -f Makefile.modules clean all`
- [ ] Verify module execution returns correct values
- [ ] Check profiling statistics are reasonable

---

## 🚀 Quick Start (For New Session)

```bash
# Navigate to project
cd /home/nickywan/dev/Git/BareFlow-LLVM

# Check current state
git status
git log --oneline -5

# Build and test
make clean && make
make run  # Should see profiling export at end

# Capture profiling data
timeout 10 qemu-system-i386 -drive file=fluid.img,format=raw \
  -serial file:/tmp/profile.json -display none

# Validate JSON
sed -n '/^{$/,/^}$/p' /tmp/profile.json | python3 -m json.tool

# Continue with next task from "Next Steps" section
```

---

## 📞 Contact & Support

For questions about this codebase, refer to:
- `CLAUDE.md` - Development guidelines
- `ROADMAP.md` - Project phases and tasks
- GitHub issues (if applicable)

**Last Updated**: 2025-10-25
**Session Focus**: Serial profiling export and bootloader fixes
**Status**: ✅ Phase 1.2 profiling export system complete
