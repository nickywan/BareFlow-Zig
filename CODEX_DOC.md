# Codex Development Playbook

## Purpose
This document anchors the BareFlow-LLVM workflow for Codex-style automation. Its goal is to help code agents understand where to look, what to run, and how to reason about the unikernel + JIT hybrid without re-reading the longer human guides.

## Key Artifacts
- `README.md` and `ROADMAP.md` narrate milestones; skim them first to grasp current focus.
- `BAREFLOW_MODULE_SYSTEM.md` explains module lifecycles and loader contracts; reference when shimming new modules.
- `JIT_MERGE_README.md` and `QUICKSTART_JIT.md` cover host-side LLVM integration and profiling interface.
- `AGENTS.md` provides contributor rituals (style, tests, PR checklists) that Codex agents must follow when generating patches.

## Implementation Notes
- Boot path: NASM in `boot/` → freestanding kernel sources in `kernel/`, linked into `build/kernel.bin`, finally packed into `fluid.img`.
- LLVM path: host tooling lives in `libs/`, `app/`, and root `Makefile.jit`; all expect LLVM 18 binaries (`clang-18`, `llvm-link-18`, `llvm-config-18`).
- Kernel tests (`kernel/*_test.c/.cpp`) execute at boot; they write directly to VGA and halt on failure, so expect QEMU output when debugging.
- Host tests (`test_jit*.cpp`) rely on `bin/` artifacts. Regenerate the minimal runtime with `make -f Makefile.jit libs` after editing `libs/minimal/*.c`.

## Workflow for Agents
1. **Recon**: open relevant docs above; run `rg` to locate symbols before editing.
2. **Edit**: prefer `apply_patch` for single-file changes; keep comments terse and informative.
3. **Build/Test**: `make` for kernel image, `make -f Makefile.jit test` for LLVM side. Use `./run.sh` for full clean rebuilds in QEMU.
4. **Validate**: capture key console lines or test summaries in the final response; do not dump entire binaries or logs.
5. **Document**: update `AGENTS.md` or this playbook when new rituals emerge so future agents inherit the context.

## Troubleshooting Cheatsheet
- Signature mismatch (`FLUD`) → re-run `make`; ensure `linker.ld` still writes the 4-byte tag in `entry.asm`.
- LLVM linker errors → verify `llvm-config-18` path and that `libs/minimal.bc` is rebuilt.
- QEMU boot hangs → check recent module changes for unchecked pointers and confirm `module_loader` offsets.
