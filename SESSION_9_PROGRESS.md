# Session 9 Progress - Runtime JIT Foundation

**Date**: 2025-10-25 (Continued)
**Status**: Phase 3.1 - Bitcode Module System ✅ 60% Complete

---

## ✅ Completed This Session

### 1. Userspace JIT Validation
```bash
make -f Makefile.jit test-interface
```
- ✅ LLVM 18 JIT works in userspace
- ✅ Function profiling functional
- ✅ Auto-reoptimization at 100 calls (O0→O1)
- ✅ 150 test iterations successful

### 2. Bitcode Module Format Designed

Created `kernel/bitcode_module.h`:
- `bitcode_header_t`: 128-byte header
  - Magic: "LLBC" (0x4C4C4243)
  - Module name (32 chars)
  - Entry function name (64 chars)
  - Bitcode size, version, opt level
- `bitcode_module_t`: Header + LLVM IR data
- API: `bitcode_load()`, `bitcode_validate()`, `bitcode_free()`

### 3. Bitcode Loader Implemented

Created `kernel/bitcode_module.c`:
- Validates magic and size
- Allocates memory for bitcode
- Returns module structure ready for JIT

### 4. Bitcode Wrapper Tool

Created `tools/wrap_bitcode.py`:
```bash
python3 tools/wrap_bitcode.py \
  --input fibonacci.bc \
  --output fibonacci_wrapped.bc \
  --name fibonacci \
  --entry module_fibonacci \
  --opt 0
```

### 5. Bitcode Compilation Tested

Generated fibonacci at all optimization levels:
```
fibonacci_O0.bc: 2472 bytes (wrapped: 2596 bytes)
fibonacci_O1.bc: 2256 bytes (wrapped: 2380 bytes)
fibonacci_O2.bc: 2256 bytes (wrapped: 2380 bytes)
fibonacci_O3.bc: 2256 bytes (wrapped: 2380 bytes)
```

### 6. Micro-JIT Architecture Designed

Created `kernel/micro_jit.h`:
- Alternative to 500KB LLVM
- Direct x86 code emission (~10KB footprint)
- Operations: MOV, ADD, SUB, CMP, JMP, JE, JNE, RET
- High-level patterns: loops, fibonacci, sum

---

## 🎯 What's Left for Phase 3

### Phase 3.1 (Bitcode Modules) - 60% Done
- ✅ Bitcode format designed
- ✅ Bitcode loader implemented
- ✅ Wrapper tool created
- ✅ Test modules compiled
- ⏳ Integrate bitcode_module.c into kernel build
- ⏳ Test bitcode loading in kernel
- ⏳ Load bitcode from FAT16 disk

### Phase 3.2 (Minimal JIT) - 20% Done
- ✅ Userspace JIT validated
- ✅ Micro-JIT architecture designed
- ⏳ Implement micro_jit.c (x86 emitter)
- ⏳ JIT compile fibonacci from bitcode
- ⏳ Execute JIT-compiled code
- ⏳ Profile execution

**Decision Point**: Micro-JIT vs Full LLVM
- **Micro-JIT**: 10KB, manual x86 emission, limited optimizations
- **Full LLVM**: 500KB, full optimizer, but needs C++ runtime

**Recommendation**: Start with Micro-JIT for PoC, upgrade to LLVM later if needed.

### Phase 3.3 (Hot-Path Recompilation) - 0% Done
- ⏳ Detect hot functions (100/1000/10000 calls)
- ⏳ Trigger background recompilation
- ⏳ Atomic code pointer swap
- ⏳ Cache management

### Phase 3.4 (Micro-JIT Implementation) - 5% Done
- ✅ API designed
- ⏳ x86 instruction emitters
- ⏳ Loop pattern compiler
- ⏳ Fibonacci pattern compiler
- ⏳ Generic pattern compiler

---

## 📊 Technical Decisions

### Bitcode Module Format
```c
// Header: 128 bytes
struct bitcode_header_t {
    uint32_t magic;           // "LLBC"
    char module_name[32];
    char entry_name[64];
    uint32_t bitcode_size;
    uint32_t version;
    uint32_t opt_level;
    uint32_t reserved[2];
};

// Total: header + bitcode data
```

### Compilation Pipeline
```bash
# Step 1: C → LLVM bitcode
clang-18 -m32 -emit-llvm -O0 -ffreestanding -nostdlib -c module.c -o module.bc

# Step 2: Wrap with BareFlow header
python3 tools/wrap_bitcode.py --input module.bc --output module_wrapped.bc \
  --name module --entry module_function --opt 0

# Step 3: Load in kernel
bitcode_load_from_disk(fs, "module_wrapped.bc", &module);

# Step 4: JIT compile (future)
micro_jit_compile(module->bitcode_data, module->header.bitcode_size);
```

### Micro-JIT vs LLVM Comparison

| Feature | Micro-JIT | Full LLVM |
|---------|-----------|-----------|
| Size | ~10KB | ~500KB |
| Optimizations | Manual patterns | Full optimizer |
| Complexity | Low | High |
| Runtime deps | None | C++ runtime, TLS |
| Compile time | Fast | Slower |
| Code quality | Good for loops | Excellent |
| Maintenance | Manual | Automatic |

**For BareFlow**: Micro-JIT is pragmatic starting point.

---

## 🔧 Implementation Notes

### Micro-JIT x86 Emitters
```c
// Example: mov eax, 42
void micro_jit_emit_mov_reg_imm(ctx, REG_EAX, 42) {
    // Emit: B8 2A 00 00 00
    emit_byte(ctx, 0xB8 + REG_EAX);
    emit_dword(ctx, 42);
}

// Example: add eax, ecx
void micro_jit_emit_add(ctx, REG_EAX, REG_ECX) {
    // Emit: 01 C8
    emit_byte(ctx, 0x01);
    emit_byte(ctx, 0xC0 + (REG_ECX << 3) + REG_EAX);
}

// Example: ret
void micro_jit_emit_ret(ctx) {
    // Emit: C3
    emit_byte(ctx, 0xC3);
}
```

### Fibonacci JIT Pattern
```c
void* micro_jit_compile_fibonacci(ctx, int n) {
    // Prologue
    // mov eax, 0   (a)
    // mov ecx, 1   (b)
    // mov edx, 0   (i)

    // loop:
    // cmp edx, n
    // jge end
    // mov ebx, eax
    // add ebx, ecx
    // mov eax, ecx
    // mov ecx, ebx
    // inc edx
    // jmp loop

    // end:
    // ret
}
```

---

## 📝 Files Created

- `kernel/bitcode_module.h` - Bitcode format definition
- `kernel/bitcode_module.c` - Bitcode loader
- `kernel/micro_jit.h` - Micro-JIT API
- `tools/wrap_bitcode.py` - Bitcode wrapper tool
- `modules/bc_fibonacci.c` - Bitcode-ready fibonacci
- `build/fibonacci_wrapped_O*.bc` - Test bitcode modules

---

## 🚧 Blockers & Challenges

### 1. C++ Runtime for LLVM
- LLVM requires C++ stdlib (~500KB)
- Thread-local storage (TLS) not implemented
- Exception handling (we use -fno-exceptions)
- Solution: Use Micro-JIT instead

### 2. x86 Instruction Encoding
- Manual encoding is tedious but doable
- Reference: Intel Software Developer Manual
- Tool: `objdump -d` for verification

### 3. Code Permissions
- JIT code must be executable
- No MMU in bare-metal (everything is RWX)
- Not a blocker for BareFlow

---

## 🎯 Next Steps (Priority Order)

### Immediate (This Week)
1. **Implement micro_jit.c** - x86 emitters
   - Start with MOV, ADD, CMP, JMP, RET
   - Test with simple function (return 42)
2. **Integrate bitcode_module into kernel**
   - Add to Makefile
   - Test loading wrapped bitcode
3. **JIT compile fibonacci**
   - Use micro-JIT to generate code
   - Execute and verify result

### Short-term (Next Week)
4. **Hot-path detection**
   - Use function_profiler for triggers
   - Recompile at 100/1000 calls
5. **Atomic code swap**
   - Replace function pointer atomically
   - Zero-downtime optimization
6. **Cache bitcode on disk**
   - Load .bc files from FAT16
   - Fallback to embedded

### Medium-term (Weeks 3-4)
7. **More JIT patterns**
   - Sum, compute_intensive, matrix operations
8. **Generic pattern compiler**
   - IR → x86 translator
   - Loop unrolling, constant folding
9. **Performance validation**
   - Compare micro-JIT vs AOT
   - Measure compilation overhead

### Long-term (Phase 4-5)
10. **Multicore support** (Phase 2.2)
    - AP startup, work distribution
11. **TinyLlama integration** (Phase 5)
    - Layer-wise JIT compilation
    - Tensor operation optimization

---

## 📚 References

- **Intel Manual**: x86 instruction encoding
- **LLVM Bitcode**: https://llvm.org/docs/BitCodeFormat.html
- **JIT Techniques**: "Engineering a Compiler" (Cooper & Torczon)
- **Micro-JIT Examples**: LuaJIT, JavaScriptCore

---

## 💡 Key Insights

1. **Micro-JIT is viable** - 10KB vs 500KB is huge win
2. **Bitcode format works** - 128-byte header + LLVM IR
3. **Userspace JIT proven** - LLVM 18 works, just too heavy for kernel
4. **Manual x86 emission doable** - Tedious but straightforward
5. **Hot-path detection ready** - function_profiler infrastructure exists

---

**Status**: Phase 3.1 ~60% complete, Phase 3.2 ~20% complete
**Next Session**: Implement micro_jit.c and test JIT execution
**Estimated Time to Working JIT**: 2-3 days of focused work
