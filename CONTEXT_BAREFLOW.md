# CONTEXT_BAREFLOW.md — Shared Context Summary

> **Purpose**: Central summary of project state, decisions, and ongoing work.
> **Rule**: Read this BEFORE any action. Update AFTER complex actions.
> **Limit**: Keep under 150 lines. Older entries → agent memos.

---

## 2025-11-01 – Migration to Zig Initiated

### Major Decision: C → Zig Migration
- [DECISION] Migrate kernel from C to Zig after 45 sessions of C issues
- [REASON] malloc corruption, return value bugs, Clang codegen errors
- [IMPACT] Complete rewrite of kernel components, maintain LLVM JIT strategy
- [REF] ZIG_MIGRATION_STRATEGY.md

### Documentation Restructuring
- [ACTION] Merged CLAUDE.md files for multi-agent workflow
- [ACTION] Updated ROADMAP.md for Phase 6 (Zig migration)
- [ACTION] Archived C/LLVM-specific documentation
- [TODO] Create initial Zig kernel structure

### Current State
- **Phase**: 6 - Zig Kernel Migration (Sessions 46-48 planned)
- **Architecture**: Unikernel, no scheduler, ring0 only
- **Strategy**: "Grow to Shrink" (60MB → 2-5MB through convergence)
- **Targets**: x86_64 (primary), aarch64 (future)

### Next Immediate Steps
1. Setup Zig build system for freestanding x86_64
2. Create minimal bootable kernel with multiboot2
3. Implement serial I/O in Zig
4. Setup FixedBufferAllocator for heap

---

## Active Agent Work

### project-overseer
- Health Check: ✅ Project restructured for Zig migration
- Next: Monitor initial Zig kernel development

### zig-kernel-developer
- TODO: Create initial kernel structure
- TODO: Port boot entry to Zig
- TODO: Implement basic drivers

### Other Agents
- Waiting for kernel base before starting their work

---

## Key Technical Decisions

1. **Allocator Strategy**: Use Zig's FixedBufferAllocator initially, migrate to GeneralPurpose later
2. **Boot Method**: Keep multiboot2/GRUB for compatibility
3. **JIT Integration**: Maintain LLVM ORC JIT, create Zig bindings
4. **llvm-libc**: Continue plan to use llvm-libc functions as IR for JIT optimization

---

## Blockers & Issues

- None currently (fresh start with Zig)

---

## Recent Accomplishments

- ✅ Documented complete C→Zig migration strategy
- ✅ Restructured documentation for multi-agent workflow
- ✅ Archived 45 sessions of C implementation attempts
- ✅ Clear roadmap for Phases 6-8

---

_Last updated: 2025-11-01 11:30 (Europe/Paris)_
_Maintainer: project-overseer_
