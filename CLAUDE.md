# CLAUDE.md — BareFlow · Workflow multi-agents (Zig kernel, LLVM/ORC JIT, llvm-libc)

_Last updated: 2025-11-01 (Migration to Zig)_

> **Purpose of this file**
> Collaboration rules for Claude Code and its sub-agents on the **BareFlow** project (scheduler-less unikernel, **Zig** kernel, **LLVM ORC** JIT, **llvm-libc** for optimizable standard functions, **x86_64** and **aarch64** targets).
> This file describes: roles, rituals, shared context structure, commit conventions, and common commands.

---

## ⚠️ CRITICAL: Project Philosophy - READ FIRST

### 🎯 "Grow to Shrink" - Size is NOT a Constraint!

**NEVER OPTIMIZE FOR INITIAL SIZE! THE SYSTEM MUST BE LARGE AT START!**

```
"On s'en fiche de la taille initiale!"

Boot 1:     [60-118MB]  FULL LLVM + app IR  → DESIRED! Profile EVERYTHING
            ↓ Universal auto-profiling
Boot 100:   [30MB]      Hot paths JIT O0→O3
            ↓ Auto-optimization
Boot 500:   [10MB]      Dead code eliminated
            ↓ Progressive convergence
Boot 1000:  [2-5MB]     Pure native export
```

### ⚡ ARCHITECTURE CHANGE: Migration to Zig (Sessions 46+)

**BareFlow is migrating from C to Zig for the kernel!**

**Why Zig** (after malloc/return value issues in Sessions 31-45):
- ✅ Built-in allocators (no more malloc nightmares)
- ✅ Comptime safety (catch errors at compile-time)
- ✅ Explicit error handling (no silent failures)
- ✅ Better LLVM integration (native IR generation)
- ✅ Freestanding support (designed for bare-metal)

**See**: `ZIG_MIGRATION_STRATEGY.md` for complete migration plan

---

## 1) Architecture Principles (Quick Reminder)

- **Unikernel**: kernel = runtime = application, **ring0 only**, **no scheduler** (no context switching)
- **Zig = kernel**: all new kernel code in Zig (freestanding, no std runtime)
- **LLVM ORC JIT**: application (e.g. TinyLlama) delivered as LLVM IR and **JIT-compiled** with tiering (O0 → O3) driven by **profiling**
- **llvm-libc**: "pure" standard functions (malloc/free, memcpy/memset, strings, math) **migrated to llvm-libc** in IR for JIT/PGO benefits
- **Persistence**: optimization cache between boots (including **arch/ISA**)
- **Targets**: first **x86_64** (QEMU → real machine), then **ARM64** (QEMU raspi3 → Raspberry Pi 4)

---

## 2) Sub-Agent Roles

Sub-agents are **mandatory** to separate responsibilities and preserve context.

- **`project-overseer` (Context Engineer)**: supervises roadmap, maintains **shared brain** (see §3), detects drift, publishes *Health Checks* (✅/⚠️/❌) and *next steps*
- **`zig-kernel-developer`**: kernel code in Zig (memory/IRQ/IO init), JIT hooks, x86_64/ARM64 portability
- **`llvm-orc-jit-specialist`**: ORC JIT integration, tiered compilation, hot paths, vectorization, cache/native snapshot
- **`zig-llvm-libc-integrator`**: standard function migration to **llvm-libc** (IR), connection to Zig kernel heap
- **`cpu-capability-specialist`**: **CPU features** detection (x86: SSE/AVX/… | ARM: NEON/SVE) and **specialization plan** for JIT
- **`application-integrator`**: app integration (IR), minimal I/O, application metrics
- **`benchmark-analyst`**: reproducible benchmarks, CSV/graphs, regression tracking
- **`documentation-and-quality`**: living docs, ADR, consistency, QEMU x86/ARM quick-start guides
- **`hardware-porting-adviser`**: ARM64 porting (kernel8 boot, MMU/caches, UART/Timer/Storage drivers), real hardware stability
- **`memory-allocator-expert`**: freestanding allocator (RW→RX for JIT, canaries, fragmentation), ORC memmgr API

> All agent prompts include **CONTEXT RULES** (see §3) and **must** reference the latest `project-overseer` summary before acting.

---

## 3) Shared Context & Token Economy

**Objective**: avoid re-entering history. Centralize decisions and memorize research.

### 3.1 Context Files

- **`CONTEXT_BAREFLOW.md`** (root): **short global summary**, read **before** any action
  Contains only what was just decided/migrated/in progress (3–6 lines per entry)
- **`kernel-zig/docs/ISSUES_RESOLVED.md`** ⚠️ **CRITICAL**: **Problems database**, read **when encountering bugs** or **before implementing new features**
  Complete list of all boot/compilation/runtime issues and their solutions (Session 46-47+)
- **`kernel-zig/docs/ARCHITECTURE_64BIT.md`** 📌 **ARCHITECTURE**: Native 64-bit explanation, read **when in doubt about 32-bit vs 64-bit**
- **`/context/agent-*.md`**: **memo per agent**, with technical details, logs, links, commands

### 3.2 Discipline (Mandatory)

- **Before acting**:
  - _read_ `CONTEXT_BAREFLOW.md` + own memo `context/agent-<me>.md`
  - ⚠️ **If encountering bugs**: IMMEDIATELY read `kernel-zig/docs/ISSUES_RESOLVED.md` BEFORE debugging
  - 📌 **If implementing new kernel features**: Read `ISSUES_RESOLVED.md` "Quick Reference" section
  - 🔍 **If unsure about 32-bit vs 64-bit**: Read `kernel-zig/docs/ARCHITECTURE_64BIT.md`
- **After complex action**:
  - Add **3–6 lines** at top of `CONTEXT_BAREFLOW.md` (dated, agent, [REF] to memo)
  - Add **details** (technical/bench/logs) at top of own memo `context/agent-<me>.md`
  - 🆕 **If resolved a NEW bug**: Add entry to `kernel-zig/docs/ISSUES_RESOLVED.md` (see format in doc)
- **Overseer**: regularly consolidate/compress `CONTEXT_BAREFLOW.md` (≤150 lines), verify each agent has logged

### 3.3 Recommended Format

```md
## 2025-11-01 – zig-kernel-developer
- [ACTION] Migrated boot entry to Zig (multiboot2 support)
- [IMPACT] Removed 500 lines of C, gained comptime safety
- [TODO] Port IDT/IRQ handlers
- [REF] context/agent-zig-kernel.md#2025-11-01
```

---

## 4) Session & Task Ritual

### 4.1 Task (PR or Unit Change)
1. Read **context** (summary + memo)
2. Implement / benchmark
3. Write **summary** + **memo**
4. Open PR (or commit) with standard message (see §6)
5. Overseer updates roadmap if needed

### 4.2 Session (Batch of Tasks)
1. Overseer publishes "**Session Plan**" (objectives, exit criteria)
2. Agents execute and log
3. Overseer publishes "**Health Check**" + *next steps* + update docs

---

## 5) Build & Run Commands

### 5.1 QEMU x86_64 (Zig freestanding, multiboot)
```bash
# Build Zig kernel (ELF multiboot) + pack ISO
zig build -Dtarget=x86_64-freestanding -Drelease-fast=true
grub-mkrescue -o bareflow.iso iso/

# Run
qemu-system-x86_64 -M q35 -cpu max -m 2048 -serial stdio -cdrom bareflow.iso
```

### 5.2 QEMU ARM64 (raspi3)
```bash
# Build aarch64 kernel
zig build -Dtarget=aarch64-freestanding -Drelease-fast=true

# Run
qemu-system-aarch64 -M raspi3 -kernel zig-out/bin/kernel8.img -serial stdio -m 1024
```

> **Note**: LLVM/ORC bare-metal integration requires JIT **memory manager** (RW→RX) + **I/D cache flush** for ARM.

---

## 6) Git Conventions & ADR

- **Branches**: `feat/<agent>/<topic>` · `fix/<agent>/<topic>` · `docs/<agent>/<topic>`
- **Commits** (examples):
  - `feat(zig-kernel): init IDT + handlers; add CPUID wrapper (x86_64)`
  - `feat(llvm-jit): ORC memmgr RW→RX; add cold->hot tiering`
  - `feat(zig-llvm-libc): wire malloc to kernel heap; memcpy IR embedded`
  - `docs(overseer): update CONTEXT_BAREFLOW + roadmap; health check`
- **ADR**: structural decisions in `docs/adr/ADR-XXXX-…md` ([REF] link in context)

---

## 7) Zig Migration Checklist (Incremental)

1. **Boot/entry** in Zig (stack, .bss zero, IDT/IRQ, timers)
2. **Heap**: freestanding allocator (FixedBuffer → General Purpose), JIT pages (RW→RX)
3. **JIT hooks** (extern stubs, symbol resolver, trampolines if needed)
4. **llvm-libc** in IR: `memcpy/memset/strlen` → `malloc/free` → (math if required)
5. **CPU features**: detection and **specialization plan** (AVX/NEON)
6. **Tiering**: O0→O3 + profiling; **persistent cache (arch/isa)**
7. **Benchmarks**: boot, JIT init, app latency/throughput, warm-boot gain
8. **ARM64 port**: I/D cache flush after JIT, MMU tables, UART/Timer/Storage

---

## 8) JIT Cache Requirements

- Record **(arch, isa, toolchain-id, module hash)**
- Reload **only** if compatible (otherwise fallback interpreted/JIT O0)
- Plan **eviction** (simple LRU) and **versioning** (cache format v1)
- Explicit save at end of run (no background task)

---

## 9) Security & Limits

- **Ring0 only**: no process isolation; careful with pointers
- No uncontrolled blocking APIs; no syscalls; no implicit alloc
- Any page marked **RX** must be **immutable** afterwards (Write-Xor-Execute)
- Limited logs in hot path (light counters, no printf in loops)

---

## 10) Quick Troubleshooting

- **Return -1 instead of 0**: check call/ret ABI and .bss zeroing; ensure `ret` follows convention (x86_64 SysV / AArch64)
- **Crash after JIT**: check ORC memmgr (alignment, RW→RX perms, ARM icache flush)
- **malloc unstable**: fix llvm-libc → Zig heap hooks; test alignments/sizes >4K
- **Slow 1st run**: tiering too aggressive → lower O0→O2 thresholds; pre-JIT 2–3 key functions

---

## 11) Critical Reference Documents

**READ BEFORE CONTINUING** (especially when resuming sessions):

### Migration Documents
1. **`ZIG_MIGRATION_STRATEGY.md`** 🔴 **CRITICAL**
   - Complete C→Zig migration strategy
   - Root causes of C failures (malloc, return values)
   - Zig advantages and 4-phase plan
   - **READ FIRST when starting any Zig work**

2. **`CONTEXT_BAREFLOW.md`** ⚠️ **ALWAYS READ**
   - Current project state summary
   - Recent decisions and actions
   - Agent work in progress
   - **READ BEFORE EVERY SESSION**

### Historical Issues (for reference)
3. **`docs/phase4/QEMU_BOOT_ISSUES_REFERENCE.md`**
   - Known QEMU boot problems and solutions
   - Useful patterns for Zig implementation

4. **`docs/phase4/SESSION_45_COMPILER_BUG_INVESTIGATION.md`**
   - Clang code generation bugs that blocked Phase 4
   - Why we're migrating to Zig

---

## 12) Document Maintenance

### Files to Keep Current
| File | Purpose | Update When |
|------|---------|-------------|
| **CLAUDE.md** | This file - AI instructions | Workflow or architecture changes |
| **CONTEXT_BAREFLOW.md** | Shared context | After each agent action |
| **ROADMAP.md** | Project timeline | Phase transitions or major milestones |
| **README.md** | Project vision | Major strategy changes only |

### Archive Policy
When documents become obsolete (C/LLVM-specific):
```bash
mkdir -p archive/docs/phase{X}
mv OLD_FILE.md archive/docs/
git add -A && git commit -m "archive: Move C/LLVM docs to archive"
```

---

## 13) Final Discipline Reminder

- Read `CONTEXT_BAREFLOW.md` **before** action
- Write **summary** + **agent memo** **after** action
- `project-overseer` is **source of truth** for context
- Don't diverge from **roadmap** without ADR + Overseer validation
- **Commit frequently** with clear messages

---

## Glossary

- **Tiered JIT**: progressive promotion (O0→O3) based on profiling
- **RW→RX**: memory pages written then made executable (JIT)
- **W^X**: Write xor Execute (never RWX in stable phase)
- **Hot path**: most frequently executed code
- **PGO**: Profile-Guided Optimization
- **Comptime**: Zig compile-time execution/validation
- **Freestanding**: No OS, no standard library

---

**Project**: BareFlow - Self-Optimizing Unikernel (Zig Migration)
**Maintainer**: Claude Code Assistant & Sub-Agents
**Human**: @nickywan
**Status**: 🚀 Phase 6 Starting - Zig Kernel Migration