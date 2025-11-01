# LLVM-libc Integration Strategy

**Created**: Session 36 (2025-10-26)
**Status**: Planned (Implementation Phase 5+)
**Goal**: Hybrid runtime combining custom bare-metal drivers + JIT-optimizable libc

---

## 🎯 Vision: Hybride kernel_lib + llvm-libc

BareFlow utilise une approche **hybride** pour maximiser à la fois la performance bare-metal ET l'optimisabilité JIT:

```
┌────────────────────────────────────────────────────────┐
│  BareFlow Runtime Strategy                             │
│  ┌──────────────────────┐  ┌──────────────────────┐   │
│  │   kernel_lib         │  │    llvm-libc         │   │
│  │   (Custom Drivers)   │  │  (JIT-Optimizable)   │   │
│  ├──────────────────────┤  ├──────────────────────┤   │
│  │  • VGA text mode     │  │  • malloc/free       │   │
│  │  • Serial I/O        │  │  • memcpy/memset     │   │
│  │  • Keyboard PS/2     │  │  • memmove           │   │
│  │  • PIC/IDT setup     │  │  • strlen/strcmp     │   │
│  │  • rdtsc/cpuid       │  │  • String functions  │   │
│  │  • Paging setup      │  │  • Math (future)     │   │
│  └──────────────────────┘  └──────────────────────┘   │
│         ↓                          ↓                   │
│   Ring 0 drivers            JIT-optimizable            │
│   Non-profileable          Auto-vectorization          │
└────────────────────────────────────────────────────────┘
```

---

## 🔧 Rationale: Pourquoi Hybride?

### ✅ kernel_lib CUSTOM pour I/O & CPU

**Raisons de garder custom:**

1. **Bare-metal spécifique**
   - VGA mode 3 (80×25 text) = accès direct 0xB8000
   - Serial port = I/O ports 0x3F8-0x3FF
   - Pas de standard POSIX/C équivalent

2. **Non-JIT-optimisable**
   - I/O ports (`outb`, `inb`) = instructions volatiles
   - MMIO (Memory-Mapped I/O) = side-effects
   - Pas de bénéfice du profiling/JIT

3. **Déjà fonctionnel**
   - kernel_lib/io testé et validé
   - 3 KB de code compact
   - Pas de dépendances

**Fonctions concernées:**
- `kernel_lib/io/vga.c` - VGA text mode
- `kernel_lib/io/serial.c` - Serial UART
- `kernel_lib/io/keyboard.c` - PS/2 keyboard
- `kernel_lib/cpu/pic.c` - Programmable Interrupt Controller
- `kernel_lib/cpu/idt.c` - Interrupt Descriptor Table
- `kernel_lib/cpu/rdtsc.c` - Cycle counter

---

### ✅ llvm-libc pour Memory & String

**Raisons de migrer vers llvm-libc:**

1. **JIT-optimisable par design**
   - Écrit en C pur, compilable en IR
   - Pas de inline assembly (sauf cas critiques)
   - LLVM peut profiler + recompiler

2. **Auto-vectorization**
   ```c
   // Boot 1: Generic implementation
   void* memcpy_generic(void* dst, const void* src, size_t n);

   // Boot 100: JIT observes: always 512 bytes aligned
   void* memcpy_avx2_512(void* dst, const void* src) {
       // AVX2 optimized for 512 bytes
       // 8× ymm registers, unrolled
       // → 10× faster!
   }
   ```

3. **Profiling universel**
   - Track ALL function calls (app + LLVM + libc)
   - Detect hot paths: `memcpy` utilisé 10 000× par inference
   - JIT compile avec spécialisation

4. **Convergence progressive**
   ```
   Boot 1:    llvm-libc generic (safe, slow)
   Boot 100:  JIT detects patterns → AVX2 memcpy
   Boot 500:  Dead functions eliminated (90% unused)
   Boot Final: Tiny optimized libc (~50 KB)
   ```

**Fonctions concernées:**
- `malloc`, `free`, `realloc`, `calloc`
- `memcpy`, `memset`, `memmove`, `memcmp`
- `strlen`, `strcmp`, `strncmp`, `strcpy`
- `strcat`, `strchr`, `strstr`
- (Future) Math: `sin`, `cos`, `sqrt`, etc.

---

## 📊 Bénéfices Attendus

### Before (kernel_lib/memory custom)

```c
// string.c: Generic memcpy
void* memcpy(void* dst, const void* src, size_t n) {
    char* d = dst;
    const char* s = src;
    while (n--) *d++ = *s++;  // 1 byte/cycle
    return dst;
}
```

**Performance**: 1 byte/cycle = **512 cycles** pour 512 bytes

### After (llvm-libc JIT-optimized)

```c
// Boot 1: llvm-libc generic (similaire)
void* memcpy(void* dst, const void* src, size_t n);  // Generic

// Boot 100: JIT observes size=512, alignment=16
void* __llvm_memcpy_512_aligned16(void* dst, const void* src) {
    // AVX2: 32 bytes/instruction
    // 16 iterations unrolled
    __m256i* d = (__m256i*)dst;
    const __m256i* s = (const __m256i*)src;
    #pragma unroll
    for (int i = 0; i < 16; i++) {
        _mm256_store_si256(d++, _mm256_load_si256(s++));
    }
    return dst;
}
```

**Performance**: 32 bytes × 16 iterations = **~50 cycles** pour 512 bytes

**Speedup**: **10× faster!** (512 cycles → 50 cycles)

---

## 📅 Migration Timeline

### Phase 5 (Sessions 36-40): Préparation

**Session 36** (actuel):
- ✅ Document this strategy
- ✅ Update CLAUDE.md, README.md
- ⏳ Refactor code for clean separation

**Session 37-39**:
- Keep using kernel_lib/memory (bump allocator)
- Focus on TinyLlama model loading
- Validate architecture works

**Session 40**:
- Prepare llvm-libc integration checklist
- Document current memory usage patterns

### Phase 6 (Sessions 41-45): llvm-libc Integration

**Session 41-42**: Build llvm-libc bare-metal
- Compile llvm-libc for x86-64 bare-metal
- Link with kernel (static library)
- Test basic malloc/free

**Session 43**: Migrate memory functions
- Replace kernel_lib/memory/malloc_bump.c → llvm-libc malloc
- Replace kernel_lib/memory/string.c → llvm-libc string
- Keep compiler_rt.c (GCC runtime support)

**Session 44**: Validation
- Test all existing functionality
- Measure memory overhead
- Profile performance baseline

**Session 45**: Documentation
- Update all references
- Document llvm-libc specifics
- Measure code size impact

### Phase 7+ (Sessions 46+): JIT Optimization

- Integrate with LLVM JIT profiler
- Observe hot llvm-libc functions
- Let JIT optimize automatically
- Measure convergence

---

## 🏗️ Implementation Details

### Directory Structure (After Migration)

```
kernel_lib/
├── io/                    # KEEP (custom drivers)
│   ├── vga.{h,c}
│   ├── serial.{h,c}
│   └── keyboard.{h,c}
├── cpu/                   # KEEP (CPU-specific)
│   ├── rdtsc.{h,c}
│   ├── cpuid.{h,c}
│   ├── pic.{h,c}
│   └── idt.{h,c}
├── memory/                # MIGRATE → llvm-libc
│   ├── compiler_rt.c      # KEEP (GCC runtime)
│   └── [REMOVED]          # malloc, string → llvm-libc
├── llvm_libc/             # NEW (llvm-libc static lib)
│   ├── lib/
│   │   └── libllvmlibc.a  # Compiled llvm-libc
│   ├── include/
│   │   ├── stdlib.h       # malloc, free, etc.
│   │   └── string.h       # memcpy, strlen, etc.
│   └── README.md          # Build instructions
└── jit/                   # KEEP (profiling)
    └── profiler.{h,c}
```

### Linking Strategy

```makefile
# Makefile
KERNEL_LIB_OBJS = \
    io/vga.o io/serial.o io/keyboard.o \
    cpu/rdtsc.o cpu/cpuid.o cpu/pic.o cpu/idt.o \
    memory/compiler_rt.o \
    jit/profiler.o

LLVM_LIBC = llvm_libc/lib/libllvmlibc.a

kernel_lib.a: $(KERNEL_LIB_OBJS)
    ar rcs $@ $^

kernel.elf: $(OBJS) kernel_lib.a $(LLVM_LIBC)
    ld -o $@ $^
```

---

## ⚠️ Risks & Mitigations

### Risk 1: llvm-libc Size Overhead

**Risk**: llvm-libc might be larger than custom kernel_lib/memory

**Mitigation**:
- Start with minimal llvm-libc (malloc + string only)
- Dead code elimination will remove unused functions
- Expect initial growth, convergence will shrink

**Acceptable**: 60 MB boot 1 → 2-5 MB boot final

### Risk 2: Bare-Metal Compatibility

**Risk**: llvm-libc might assume OS services (syscalls, etc.)

**Mitigation**:
- Use llvm-libc "fullbuild" mode (no syscalls)
- Provide custom entry points if needed
- Test incrementally (malloc first, then string)

**Fallback**: Keep kernel_lib custom if incompatible

### Risk 3: Performance Regression (Boot 1-10)

**Risk**: Generic llvm-libc slower than hand-tuned kernel_lib

**Mitigation**:
- Acceptable for "Grow to Shrink" philosophy
- Boot 1-10 = profiling phase (speed not critical)
- JIT will optimize by Boot 100

**Expectation**: Boot 1 slower, Boot 100+ much faster

---

## 📚 References

### llvm-libc Documentation
- https://libc.llvm.org/
- https://github.com/llvm/llvm-project/tree/main/libc
- Build instructions: https://libc.llvm.org/build_and_test.html

### BareFlow Design Documents
- `README.md` - Vision "Grow to Shrink"
- `CLAUDE.md` - Runtime strategy
- `docs/phase3/PHASE3_*.md` - JIT validation results

### Related Sessions
- Session 35: Return crash debug (led to architecture revision)
- Session 25: Enhanced allocator (200 MB heap)
- Session 31: malloc investigation (paging + bump allocator)

---

**Status**: DOCUMENTED ✅
**Next**: Update CLAUDE.md + README.md to reference this strategy
**Implementation**: Phase 6 (Sessions 41-45)
