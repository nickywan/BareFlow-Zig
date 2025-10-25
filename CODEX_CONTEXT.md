# CODEX Context – BareFlow-LLVM

## Mission Snapshot
Fluid OS targets a single-purpose, bare-metal unikernel that runs one module in ring 0 and self-optimizes through LLVM-driven recompilation. The host machine (LLVM 18 + llvm-libc) performs heavyweight builds, while the kernel stays freestanding and lean. Phase 1.2 concentrates on exporting profiler data, replaying it on the host, and feeding optimized modules back into the boot image.

## Current Branch State
- Two-stage NASM bootloader now hands off cleanly: the protected-mode trampoline was changed from `call KERNEL_OFFSET` to `jmp KERNEL_OFFSET + 4`, so the CPU skips the `FLUD` signature and executes the real `_start` code.
- Kernel entry zeroes the full `.bss` span before C runs, fixing the random heap offsets that previously caused aggressive out-of-range writes (`timeout` + `-d guest_errors` now stops spamming >0x8000000 accesses).
- `kernel_main` emits a serial debug line right after `idt_init()`, but the COM1 channel still stays silent under `-serial stdio`; need to verify whether the UART status bit never flips or if the loopback test needs relaxing.
- GDB session (`target remote | qemu … -gdb stdio -S`) confirms we reach `kernel_main` and step into module tests; the hang happens later in the VGA flow once QEMU is allowed to run.
- Serial JSON export path is hooked but unvalidated on this branch because the boot run never gets far enough to print to COM1.

## High-Priority Fixes
1. `kernel/profiling_export.c` — Investigate why `serial_puts` returns early (likely `serial_is_transmit_empty()` timing out). Relax the poll or add an LSR warm-up so early debug strings surface on `-serial stdio`.
2. `kernel/jit_allocator.c:143-151` — Align-first strategy still fails on fresh pools; we need the planned padding/realignment fix before allocator tests start chewing uninitialised memory again.
3. `boot/stage2.asm:200-247` — Double-check Disk Address Packet math once more with `objdump` to ensure the `.rodata` tail stays loaded as the kernel grows (currently capped at 128 sectors = 64 KiB).

## Debug & Test Rituals
- Build with `make LIGHT=1` during debugging to shrink the workload. Run QEMU via `timeout --signal=INT --kill-after=5 120 qemu-system-i386 -drive file=fluid.img,format=raw -serial stdio -display none` to capture COM1 logs in the terminal.
- Use `gdb -q build/kernel.elf` + `target remote | qemu … -gdb stdio -S` to break at `kernel_main`, `module_execute`, and `profiling_trigger_export` without needing VGA output.
- Once serial is unstuck, capture the profiling JSON payload, feed it into `tools/pgo_recompile.py --apply`, and reboot to confirm cached modules load.
- Before closing any session, sync findings to `CODEX_REVUE.md` (major issues/questions) and update `ROADMAP.md` checkpoints.

## Open Questions
- What is the minimal deliverable for roadmap task 1.2: serial export alone, or do we need the full offline recompilation loop working end-to-end?
- Should `fluid.img` stay within 64 KiB of kernel sectors, or can we expand the loader budget as modules and cached code grow?
- When TinyLlama integration begins, do we plan to jump to long mode early to gain address-space headroom for weights and caches?
