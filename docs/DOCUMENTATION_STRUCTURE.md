# BareFlow Documentation Structure

**Last Updated**: 2025-10-26
**Purpose**: Quick reference for documentation organization

---

## 📁 Root Level Documents (8 files)

Essential files that must remain at root for immediate access:

| File | Purpose | Update Frequency |
|------|---------|-----------------|
| **CLAUDE.md** | AI assistant instructions & session recovery | When workflow changes |
| **ROADMAP.md** | Project phases, current tasks, timeline | Every session |
| **README.md** | Project vision, "Grow to Shrink" strategy | Major changes only |
| **ARCHITECTURE_UNIKERNEL.md** | Detailed unikernel architecture | When design changes |
| **BOOTLOADER_NOTES.md** | Boot process technical details | When boot issues occur |
| **LLVM_PIPELINE.md** | 4-phase optimization strategy | Phase transitions |
| **JIT_ANALYSIS.md** | JIT implementation analysis | When JIT approach changes |
| **CHANGELOG.md** | Version history | Major releases |

---

## 📂 docs/ Directory

Reference documentation and results:

```
docs/
├── phase3/                           # Phase 3 validation results
│   ├── PHASE3_2_FINDINGS.md         # Static linking research
│   ├── PHASE3_3_RESULTS.md          # 399× speedup proof
│   ├── PHASE3_4_TIERED_JIT.md       # Tiered compilation
│   ├── PHASE3_5_DCE_RESULTS.md      # 99.83% dead code
│   └── PHASE3_6_NATIVE_EXPORT.md    # 6000× size reduction
├── PROJECT_COHERENCE_REPORT.md      # Project analysis (95/100 score)
└── DOCUMENTATION_STRUCTURE.md       # This file
```

---

## 📦 archive/ Directory

Historical or obsolete documentation:

```
archive/
├── docs/                             # Archived documentation
│   ├── CLAUDE_CONTEXT.md           # Old session context (merged into CLAUDE.md)
│   ├── NEXT_SESSION.md             # Old next steps (replaced by ROADMAP.md)
│   ├── SESSION_SUMMARY.md          # Historical sessions
│   ├── CODE_REUSE_MAP.md          # Reuse analysis (complete)
│   └── ARCHITECTURE_DECISIONS.md   # Decision process (complete)
├── monolithic_kernel/               # Old kernel code (346KB version)
│   ├── kernel/                     # Monolithic kernel source
│   └── modules/                    # Old module system
└── cleanup_project.sh              # Used organization script
```

---

## 🔄 Session Recovery Process

When starting a new Claude session:

1. **Read Core Docs** (in order):
   - CLAUDE.md → Instructions
   - ROADMAP.md → Current state
   - README.md → Vision

2. **Check Phase**:
   - Phase 4: Read docs/phase3/*.md for context
   - Phase 5+: Read ARCHITECTURE_UNIKERNEL.md

3. **Update After Work**:
   - ROADMAP.md → Mark completed tasks
   - CLAUDE.md → Update if commands changed
   - Create docs/phaseX/SESSION_Y_RESULTS.md if major progress

---

## 📊 Documentation Metrics

- **Root Level**: 8 essential files (streamlined from 18)
- **Archived**: 7 obsolete files
- **Phase 3 Results**: 5 validation documents
- **Total Reduction**: 44% fewer files at root
- **Redundancy Eliminated**: CLAUDE_CONTEXT.md merged into CLAUDE.md

---

## ✅ Benefits of New Structure

1. **No Redundancy**: Single source of truth for each topic
2. **Clear Hierarchy**: Essential at root, details in subdirs
3. **Easy Recovery**: 3-step process in CLAUDE.md
4. **Version Control**: Historical docs preserved in archive/
5. **Phase-Specific**: Results organized by phase number

---

**Maintainer**: Claude Code Assistant
**Human**: @nickywan