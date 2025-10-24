# Repository Guidelines

## Project Structure & Module Organization
- `boot/` holds the stage1/2 NASM boot code that seeds the kernel load.
- `kernel/` contains 32-bit freestanding C/C++ sources, tests, and linker script.
- `app/` provides LLVM IR demo programs; `modules/` holds dynamic module samples.
- `libs/` stores host-side minimal runtimes (compiled into `libs/minimal.bc`).
- `build/` is generated output (binaries, bitcode); keep it out of commits.

## Toolchain & Environment
- Build expects `clang-18`, `clang++-18`, `llvm-link-18`, `llvm-config-18`, `nasm`, `ld`, and `qemu-system-i386`.
- Verify your LLVM setup before building: `llvm-config-18 --version`.
- Use a Unix-like shell; scripts assume GNU coreutils.

## Build, Test, and Development Commands
- `make` builds the bootloader, kernel, and assembles `fluid.img`.
- `make run` launches QEMU with the freshly built image for an integration check.
- `make debug` starts QEMU with verbose CPU/interrupt tracing.
- `make -f Makefile.jit all` builds host-side JIT binaries in `bin/`.
- `make -f Makefile.jit test` runs `test_jit` and `test_jit_interface` against LLVM 18.
- `./run.sh` is a clean build + run helper when iterating rapidly.

## Coding Style & Naming Conventions
- Follow kernel sources: 4-space indentation, no tabs, and ANSI `//` comments.
- Use `snake_case` for C functions and variables; reserve PascalCase for enums or structs that represent types.
- Keep kernel code freestanding-friendly (`-ffreestanding`, no libc); avoid exceptions/RTTI in C++ (`-fno-exceptions`, `-fno-rtti` already enforced).
- Assembly files are NASM syntax; align labels to column 0 and document control flow with short comments.

## Testing Guidelines
- Host JIT coverage lives in `test_jit*.cpp`; extend by adding new `_test.cpp` files and wiring them into `Makefile.jit`.
- Kernel self-tests reside under `kernel/*_test.c` and execute during boot; gate new checks behind clear console banners so regressions are obvious in QEMU output.
- When modifying modules, rebuild `libs/minimal.bc` (`make -f Makefile.jit libs`) so tests link the updated bitcode.
- Record boot logs or screenshots when validating changes that require QEMU output.

## Commit & Pull Request Guidelines
- Commit messages are short, imperative, and scoped (e.g., `Add JIT memory allocator…`); include brackets only for temporary qualifiers like `[WIP: ...]`.
- Reference affected subsystems (`boot`, `kernel`, `jit`) early in the subject to aid history searches.
- Pull requests should describe the change, list manual/automated tests run, link any roadmap issue, and attach relevant QEMU console snippets for kernel-visible changes.
