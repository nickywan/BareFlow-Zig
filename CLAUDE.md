# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 📌 Quick Context Recovery

**For continuing a previous session**, read `CLAUDE_CONTEXT.md` first - it contains:
- Current project state and recent changes
- Last session's work and commits
- Known issues and solutions
- Exact next steps to take

## Project Overview

**Fluid OS** is a bare-metal unikernel with LLVM-based runtime optimization capabilities. The system boots from a two-stage bootloader into a 32-bit protected mode kernel that supports dynamic module loading and profiling.

## Build Commands

### Bare-Metal Kernel
```bash
# Build the complete bootable kernel
make                  # Build everything (stage1, stage2, kernel)
make run              # Build and launch in QEMU with serial output
make debug            # Build and run with CPU debug logs
make clean            # Remove all build artifacts
make rebuild          # Clean + build
```

### JIT Interface (LLVM 18)
```bash
# Requires: llvm-18-dev, clang-18, llvm-18-tools
make -f Makefile.jit info           # Show LLVM configuration
make -f Makefile.jit all            # Build JIT tests and minimal.bc library
make -f Makefile.jit libs           # Build only minimal.bc (LLVM bitcode library)
make -f Makefile.jit test-basic     # Test basic LLVM LLJIT
make -f Makefile.jit test-interface # Test JIT interface with profiling
make -f Makefile.jit clean          # Clean JIT build artifacts
```

### Module System (AOT Compiled Modules)
```bash
make -f Makefile.modules all          # Compile all modules in modules/ to .mod files
make -f Makefile.modules list-modules # List compiled modules with sizes
make -f Makefile.modules clean        # Clean module build artifacts
```

### Testing
```bash
# Userspace module system validation
gcc -O2 -I. test_modules_userspace.c -o test_modules_userspace
./test_modules_userspace
```

## Architecture

### Boot Process
The system uses a **two-stage bootloader** to transition from BIOS to 32-bit protected mode:

1. **Stage 1** (`boot/stage1.asm`): 512-byte MBR bootloader loaded by BIOS at 0x7C00
   - Loads Stage 2 from sectors 1-8 to 0x7E00
   - Minimal functionality, CHS disk access only

2. **Stage 2** (`boot/stage2.asm`): 4KB extended bootloader at 0x7E00
   - Enables A20 line (BIOS, keyboard controller, or Fast A20 methods)
   - Loads kernel from sector 9+ to 0x1000 using LBA (with CHS fallback)
   - Verifies kernel signature ("FLUD" magic bytes: 0x464C5544)
   - Sets up GDT with flat 4GB segments (code at 0x08, data at 0x10)
   - Transitions to protected mode and jumps to kernel at 0x1000

3. **Kernel Entry** (`kernel/entry.asm`): Assembly entry point
   - Must start with "FLUD" signature (validated by Stage 2)
   - Sets up initial stack at 0x90000
   - Calls `kernel_main()` in C

### Disk Layout
```
Sector 0:       Stage 1 bootloader (512 bytes)
Sectors 1-8:    Stage 2 bootloader (4096 bytes)
Sectors 9+:     Kernel binary (variable size)
```

### Module System Architecture

The kernel supports **two approaches** for dynamic code execution:

#### 1. AOT Module System (Currently Implemented)
- **Location**: `kernel/module_loader.{h,c}`, `kernel/embedded_modules.h`
- **Approach**: Pre-compile modules with LLVM -O2, load native code at runtime
- **Why AOT**: Real JIT (LLVM OrcJIT) requires C++ runtime, allocator, threads, system libraries - impractical for bare-metal
- **Key Features**:
  - Modules compiled offline with `clang-18 -m32 -ffreestanding -O2`
  - Native code extracted with `objcopy -O binary`
  - Cycle-accurate profiling using `rdtsc` instruction
  - Tracks: call count, total/min/max/avg cycles per module
  - Embedded modules: `fibonacci`, `simple_sum`, `compute_intensive`, `count_primes`
- **Module Loading**: `module_load()` validates and stores function pointers
- **Execution**: `module_execute()` wraps calls with rdtsc profiling

#### 2. JIT Interface (In Progress)
- **Location**: `kernel/jit_interface.h`, `kernel/jit_llvm18.cpp`
- **Purpose**: LLVM 18 LLJIT wrapper for userspace testing and future kernel integration
- **Features**:
  - C interface with opaque types for bare-metal compatibility
  - Profile-guided optimization: auto-recompile after 100 calls (O0→O1) and 1000 calls (O1→O2)
  - Function profiling with cycle tracking
  - Multi-level optimization support (JIT_OPT_NONE, JIT_OPT_BASIC, JIT_OPT_AGGRESSIVE)
- **Status**: Working in userspace (test_jit_interface.cpp), bare-metal integration pending
- **Challenge**: Requires substantial runtime support (C++ stdlib, allocator, exceptions)

### Key Components

#### Kernel Core (`kernel/kernel.c`)
- Entry point: `kernel_main()`
- Initializes VGA text mode, keyboard, and module system
- Runs 4 automated module tests at boot
- Prints detailed profiling statistics
- Uses keyboard pause system between test sections

#### VGA Driver (`kernel/vga.{h,c}`)
- Text mode 80x25 at 0xB8000
- 16-color text with foreground/background control
- Functions: `terminal_initialize()`, `terminal_putchar()`, `terminal_writestring()`, `terminal_setcolor()`

#### Keyboard (`kernel/keyboard.h`)
- PS/2 keyboard input via port 0x60
- `wait_key()` function for user interaction
- Used for "press any key to continue" functionality

#### Standard Library (`kernel/stdlib.{h,c}`)
- Bare-metal implementations: `memset`, `memcpy`, `strlen`, `strcmp`
- Simple `malloc`/`free` with 64KB heap at 0x100000
- No libc dependencies

#### Linker Script (`kernel/linker.ld`)
- Places kernel at 0x1000 physical address
- Section order: .text, .rodata, .data, .bss
- Aligns sections to 4096-byte boundaries

## Development Workflow

### Adding a New Kernel Feature
1. Modify `kernel/kernel.c` or add new files in `kernel/`
2. Update `Makefile` if adding new C files (add to kernel dependencies and link order)
3. Build with `make` and test with `make run`
4. Use `make debug` for detailed CPU state on errors

### Creating a New Module
1. Write module in `modules/yourmodule.c` (must have a parameterless int-returning function)
2. Compile: `make -f Makefile.modules`
3. To embed in kernel, add to `kernel/embedded_modules.h`:
   - Declare as `extern uint8_t module_yourmodule_data[]`
   - Add to `embedded_modules` array
   - Increment `NUM_EMBEDDED_MODULES`
4. Call from kernel with `module_execute(&mgr, "yourmodule")`

### Working with JIT Interface
1. Modify interface in `kernel/jit_interface.h` (C header)
2. Implement in `kernel/jit_llvm18.cpp` (C++ implementation)
3. Test userspace first: `make -f Makefile.jit test-interface`
4. Bare-metal integration requires custom allocator and C++ runtime stubs

### Debugging Boot Issues
- Error codes displayed by Stage 2:
  - `0xD1` = Disk read failure
  - `0xD2` = Incorrect sector count
  - `0xA2` = A20 line cannot be enabled
  - `0x51` = Invalid kernel signature (expected "FLUD")
  - `0xE0` = Kernel not found
- Use `make debug` to see CPU state and interrupt logs
- Verify kernel signature: `hexdump -n 4 -e '4/1 "%02x"' build/kernel.bin` should show `44554c46`

## Important Constraints

### Bare-Metal Limitations
- **No standard library**: All C runtime functions must be implemented in `kernel/stdlib.c`
- **No dynamic allocator** for C++: JIT requires custom memory management
- **No exceptions**: Compile with `-fno-exceptions` for bare-metal C++
- **32-bit only**: All code compiled with `-m32`
- **No floating point**: Use `-mno-sse -mno-mmx` if adding FP later

### Bootloader Constraints
- Stage 1 must be exactly 512 bytes (enforced by Makefile)
- Stage 2 must be exactly 4096 bytes (8 sectors)
- Kernel must start with "FLUD" signature (validated before execution)
- Stack grows down from 0x7C00 in Stage 1, relocated to 0x90000 in kernel

### Module System
- AOT modules must be compiled with `-m32 -ffreestanding -nostdlib`
- Module functions must match signature: `int function_name(void)`
- Maximum 16 modules (MAX_MODULES in module_loader.h)
- Module names limited to 32 characters

## Code Organization

```
BareFlow/
├── boot/
│   ├── stage1.asm         # MBR bootloader (512 bytes)
│   └── stage2.asm         # Extended bootloader (4KB)
├── kernel/
│   ├── entry.asm          # Kernel entry point with "FLUD" signature
│   ├── kernel.c           # Main kernel code and module tests
│   ├── linker.ld          # Linker script (kernel at 0x1000)
│   ├── vga.{h,c}          # VGA text mode driver
│   ├── keyboard.h         # PS/2 keyboard interface
│   ├── stdlib.{h,c}       # Bare-metal C library
│   ├── module_loader.{h,c}      # AOT module system
│   ├── embedded_modules.h       # Embedded module declarations
│   ├── jit_interface.h          # JIT C interface
│   └── jit_llvm18.cpp           # LLVM 18 JIT implementation
├── modules/
│   ├── fibonacci.c        # Example: Fibonacci calculation
│   ├── simple_sum.c       # Example: Sum 1-100
│   ├── compute.c          # Example: Nested loop computation
│   └── *.mod              # Compiled native modules (generated)
├── libs/
│   ├── minimal/           # Minimal C library for JIT (strlen, memcpy, etc.)
│   └── minimal.bc         # LLVM bitcode library (generated)
├── Makefile               # Main kernel build system
├── Makefile.jit           # JIT interface build system
├── Makefile.modules       # Module compilation system
└── build/                 # Build artifacts (generated)
```

## Performance Notes

### Module Profiling
The `rdtsc` instruction provides cycle-accurate measurements. Typical cycle counts (userspace reference):
- `simple_sum`: ~64 cycles
- `fibonacci`: ~92 cycles
- `count_primes`: ~19,276 cycles
- `compute_intensive`: ~27,392 cycles (average over 10 runs)

Profiling overhead is minimal (~10-20 cycles for rdtsc + counter updates).

### JIT Optimization Thresholds
When using the JIT interface:
- 0-99 calls: No optimization (O0)
- 100-999 calls: Basic optimization (O1) - 2-3x speedup
- 1000+ calls: Aggressive optimization (O2/O3) - 5-10x speedup

## Related Documentation

- `README.md`: High-level overview and quick start
- `BAREFLOW_MODULE_SYSTEM.md`: Detailed module system documentation (French)
- `JIT_MERGE_README.md`: JIT interface merge details and architecture
- `QUICKSTART_JIT.md`: Step-by-step JIT setup and testing guide
- `CHANGELOG.md`: Version history
