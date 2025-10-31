# Return Value Corruption Bug - Reference

**Status**: 🔍 UNDER INVESTIGATION
**First Observed**: Session 37 (2025-10-26)
**Affected**: tinyllama_create_model() and similar functions
**Environment**: Clang-18, x86-64 bare-metal, -O0

## Symptom

Function returns `0` but caller receives `-1`:

```c
int tinyllama_create_model(TinyLlamaModel** model) {
    // ... successful allocation ...
    serial_puts("[C_FUNC] About to return 0\n");  // Printed
    return 0;  // Function returns 0
}

// In caller:
int result = tinyllama_create_model(&model);
serial_puts("[DEBUG] result = ");
if (result == 0) serial_puts("0");      // NOT printed
else if (result == -1) serial_puts("-1");  // PRINTED!
```

## Observed Behavior

### What Works
- ✅ Model structure allocated correctly
- ✅ Pointer `model` is valid (not NULL)
- ✅ All memory allocations succeed
- ✅ Serial output shows "About to return 0"

### What Fails
- ❌ Return value corrupted (0 → -1)
- ❌ Caller receives -1 instead of 0
- ❌ Blocks normal control flow

## Compilation Details

### Current Configuration
```makefile
CC = clang-18
CFLAGS = -target x86_64-unknown-none \
         -ffreestanding -nostdlib -fno-pie -O0 \
         -fno-stack-protector -mno-red-zone -mcmodel=kernel \
         -fcf-protection=none
LD = ld.lld-18
LDFLAGS = -T linker.ld -nostdlib
```

### Why -O0?
- -O1/-O2 break malloc() completely
- -O0 fixes malloc but return value still corrupted
- Likely ABI or calling convention issue

## Timeline

**Session 37**: First appearance with -O1/-O2
- Malloc failures at LN1 layer
- Return value corruption observed

**Session 38**: Switched to -O0
- Malloc fixed
- Return value bug persists
- Model functional despite bug

**Sessions 39-40**: Work continues with workaround
- Inference pipeline implemented
- Weight loading prepared
- Using pointer check instead of return value

## Hypotheses

### 1. ABI Mismatch (Most Likely)
- x86-64 SysV ABI uses RAX for return values
- Bare-metal environment may have different expectations
- Clang possibly not preserving RAX correctly

### 2. Stack Alignment Issue
- Red zone disabled (-mno-red-zone)
- Stack may be misaligned at function exit
- Return value could be written to wrong location

### 3. Calling Convention
- -mcmodel=kernel affects code generation
- May change how return values are passed
- Possible conflict with bare-metal startup code

### 4. Register Clobbering
- Function may be clobbering RAX accidentally
- Compiler optimization (even at -O0) might reuse RAX
- No clear evidence yet

## Workaround (Current)

```c
// Instead of:
if (tinyllama_create_model(&model) == 0) {
    // Success path
}

// Use:
tinyllama_create_model(&model);
if (model != NULL) {
    // Success - model is valid
    // Ignore return value
}
```

## Investigation Plan (Session 41+)

### Option 3: GCC Diagnostic
**Goal**: Use GCC **as diagnostic tool only** (NOT production!)

1. **Compile with GCC**:
   ```bash
   gcc -m64 -ffreestanding -nostdlib -O0 ...
   ```

2. **Compare Assembly**:
   ```bash
   objdump -d kernel_clang.o > clang_asm.txt
   objdump -d kernel_gcc.o > gcc_asm.txt
   diff -u clang_asm.txt gcc_asm.txt
   ```

3. **Check RAX Handling**:
   - Look for `mov %rax, ...` before `ret`
   - Verify stack operations around return
   - Compare function prologues/epilogues

4. **GCC Warnings**:
   - GCC may emit ABI warnings Clang doesn't
   - Check for calling convention messages
   - Look for stack alignment warnings

### Alternative Fixes (If GCC reveals issue)

**Fix 1**: Explicit Register Constraints
```c
int tinyllama_create_model(TinyLlamaModel** model) {
    // ... code ...
    int result = 0;
    __asm__ volatile("" : "=a"(result) : "0"(result));
    return result;
}
```

**Fix 2**: Volatile Return Value
```c
volatile int tinyllama_create_model(...) {
    volatile int ret = 0;
    return ret;
}
```

**Fix 3**: Out Parameter
```c
void tinyllama_create_model(TinyLlamaModel** model, int* result) {
    // ... code ...
    *result = 0;  // Via pointer instead of return
}
```

## Important Notes

### GCC is NOT the Solution!
- **Use**: Diagnostic tool to understand bug
- **Analyze**: Assembly differences
- **Identify**: Root cause
- **Fix**: In Clang-compatible way
- **Stay**: On LLVM for production (JIT requirement)

### Why Not Switch to GCC?
1. **LLVM JIT Required**: "Grow to Shrink" needs LLVM OrcJIT
2. **Project Goal**: Self-optimizing system with LLVM
3. **GCC Limitations**: No runtime JIT compilation
4. **Consistency**: Keep one toolchain (Clang + LLVM)

## Related Files

- `tests/phase4/qemu_llvm_64/tinyllama_model.c:300` - tinyllama_create_model()
- `tests/phase4/qemu_llvm_64/kernel.c:91` - Caller with debug output
- `tests/phase4/qemu_llvm_64/Makefile` - Build configuration
- `docs/phase4/SESSION_33_MALLOC_DEBUG.md` - Related malloc investigation

## References

- x86-64 SysV ABI Specification
- Clang CodeGen for Freestanding Targets
- LLVM Calling Conventions Documentation
- Bare-Metal x86-64 Programming Guide

---

**Last Updated**: 2025-10-26
**Status**: Investigation ongoing (Session 41)
**Workaround**: Use pointer check instead of return value
