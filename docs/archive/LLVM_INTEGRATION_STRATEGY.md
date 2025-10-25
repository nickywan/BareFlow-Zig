# LLVM Integration Strategy for BareFlow

**Date**: 2025-10-25
**Context**: Session 13 - LLVM ORC JIT Integration Planning
**Problem**: How to integrate LLVM IR execution in a 64KB kernel constraint

---

## The Challenge

### Current State
- **Kernel size**: 57KB (112 sectors)
- **Bootloader capacity**: 64KB (128 sectors)
- **Remaining space**: 7KB

### LLVM Static Linking Analysis
Attempting to statically link LLVM ORC JIT into kernel:

```
libLLVMOrcJIT.a:          5.1 MB
libLLVMCore.a:            8.5 MB
libLLVMExecutionEngine.a: 277 KB
libLLVMCodeGen.a:         18 MB
libLLVMSupport.a:         (multiple MB)
libLLVMTarget.a:          (multiple MB)
libLLVMMC.a:              2.6 MB
libLLVMObject.a:          3.5 MB
-----------------------------------
TOTAL:                    >40 MB
```

**Conclusion**: Static linking is **IMPOSSIBLE** with current bootloader.

---

## Strategic Options

### Option 1: Increase Bootloader Capacity ❌

**Approach**: Expand bootloader to 512+ sectors (256KB+)

**Pros**:
- Can statically link LLVM
- Full JIT compilation capability
- Best performance for compiled code

**Cons**:
- Complex bootloader segment management (already tried, failed)
- Still limited by disk loading time
- Goes against user directive: "runtime optimization > kernel size"
- Kernel becomes monolithic, not modular

**Verdict**: NOT RECOMMENDED

---

### Option 2: Disk-Based Module Loading ❌

**Approach**: Load LLVM libraries from disk at runtime

**Pros**:
- Kernel stays small
- Can load LLVM incrementally

**Cons**:
- No MMU for dynamic linking
- Bare-metal has no ELF loader
- Symbol resolution complex without libc
- Still need to compile LLVM for i386 freestanding

**Verdict**: TOO COMPLEX for bare-metal

---

### Option 3: LLVM IR Interpreter ✅ RECOMMENDED

**Approach**: Write or port a lightweight LLVM IR interpreter

**Pros**:
- Small footprint: ~50-100KB estimated
- Fits in current kernel with room to spare
- Direct bitcode execution without x86 codegen
- Good enough performance for cold paths
- Hot paths can use Micro-JIT for speed

**Cons**:
- Slower than compiled code (10-100x depending on code)
- Need to implement IR instruction semantics
- No aggressive optimizations

**Implementation Paths**:

#### 3A: Port Existing LLVM Interpreter
LLVM has a built-in interpreter in `libLLVMExecutionEngine`:
- File: `llvm/lib/ExecutionEngine/Interpreter/`
- Size: ~30KB of code
- Dependencies: LLVMCore (8.5MB) - **TOO BIG**

#### 3B: Write Minimal IR Interpreter
Custom lightweight interpreter:
- Parse bitcode format (LLVM bitstream)
- Implement core instructions (load, store, arithmetic, branches)
- Stack-based execution
- Estimated size: 20-50KB

#### 3C: Hybrid Approach ✅ **BEST OPTION**
Combine Micro-JIT with selective interpretation:

1. **Parse bitcode** → extract function IR
2. **Pattern match** common patterns (loops, arithmetic)
3. **Micro-JIT compile** hot patterns to x86
4. **Interpret** complex/cold IR instructions
5. **Adaptive JIT** progressively compiles more as functions get hot

---

## Recommended Implementation: Hybrid Micro-JIT + IR Interpreter

### Architecture

```
Bitcode File (.bc)
    ↓
┌─────────────────────────────────────┐
│   Bitcode Parser                    │
│   - Read LLVM bitstream format      │
│   - Extract function IR             │
│   - Build simplified IR             │
└─────────────────┬───────────────────┘
                  ↓
    ┌─────────────────────────┐
    │  Pattern Analyzer       │
    │  - Detect loops         │
    │  - Detect arithmetic    │
    │  - Detect fibonacci,    │
    │    sum, primes, etc.    │
    └─────┬───────────────┬───┘
          ↓               ↓
  ┌───────────────┐   ┌──────────────┐
  │  Micro-JIT    │   │  Interpreter │
  │  (hot paths)  │   │  (cold code) │
  └───────────────┘   └──────────────┘
          ↓               ↓
      x86 Code        Stack Machine
          ↓               ↓
     ┌──────────────────────┐
     │   Adaptive JIT       │
     │   - Profile calls    │
     │   - Recompile hot    │
     │   - Atomic swap      │
     └──────────────────────┘
```

### Components

#### 1. Bitcode Parser (kernel/bitcode_parser.{h,c})
**Size estimate**: ~15KB

```c
typedef struct {
    uint32_t opcode;        // LLVM instruction opcode
    uint32_t num_operands;
    uint32_t operands[8];   // Operand indices
} ir_instruction_t;

typedef struct {
    char name[64];
    uint32_t num_instructions;
    ir_instruction_t* instructions;
    uint32_t num_basic_blocks;
    // Simplified IR representation
} ir_function_t;

// Parse bitcode into simplified IR
int bitcode_parse(const uint8_t* bitcode, size_t size, ir_function_t** out);
```

**Features**:
- Parse LLVM bitstream wrapper
- Extract function definitions
- Build simplified IR (subset of LLVM IR)
- Support basic types: i32, i64, float, pointers

#### 2. IR Interpreter (kernel/ir_interpreter.{h,c})
**Size estimate**: ~20KB

```c
typedef struct {
    uint32_t registers[256];  // Virtual registers
    uint8_t* stack;           // Function stack
    uint32_t stack_ptr;
} ir_execution_context_t;

// Execute IR function
int ir_execute(ir_function_t* func, uint32_t* args, uint32_t num_args, uint32_t* result);
```

**Supported Instructions** (minimal set):
- **Arithmetic**: add, sub, mul, div, rem
- **Logic**: and, or, xor, shl, shr
- **Memory**: load, store, alloca
- **Control flow**: br, ret, call
- **Comparison**: icmp (eq, ne, slt, sle, sgt, sge)
- **Conversion**: trunc, zext, sext, bitcast

#### 3. Pattern-Based JIT Bridge (kernel/pattern_jit_bridge.{h,c})
**Size estimate**: ~10KB

```c
typedef enum {
    PATTERN_FIBONACCI,
    PATTERN_SUM,
    PATTERN_PRIMES,
    PATTERN_LOOP_SIMPLE,
    PATTERN_UNKNOWN
} ir_pattern_type_t;

// Analyze IR to detect patterns
ir_pattern_type_t pattern_detect(ir_function_t* func);

// Compile pattern to x86 using Micro-JIT
void* pattern_compile(ir_pattern_type_t pattern, ir_function_t* func);
```

**Pattern Detection**:
- Analyze control flow graph
- Match against known patterns
- Extract parameters (loop bounds, initial values, etc.)
- Generate optimized x86 code

#### 4. Integration with Adaptive JIT (kernel/adaptive_jit.c)
**Modification**: ~5KB additional code

```c
// Enhanced to handle bitcode modules
typedef struct {
    char name[64];
    bitcode_module_t* bitcode;    // Bitcode module
    ir_function_t* ir_func;        // Parsed IR
    void* code_ptr;                // Compiled code (x86)
    adaptive_jit_state_t state;    // O0/O1/O2/O3
    ir_pattern_type_t pattern;     // Detected pattern
    bool is_interpreted;           // true = interpreter, false = JIT
} adaptive_module_t;
```

**Workflow**:
1. **Cold (0 calls)**: Interpret IR directly (slow but functional)
2. **Warm (100+ calls)**: Detect pattern, compile with Micro-JIT if matched
3. **Hot (1000+ calls)**: Optimize compiled code (better register allocation, loop unrolling)
4. **Very Hot (10000+ calls)**: Aggressive optimization (inlining, vectorization if pattern supports)

---

## Implementation Plan

### Phase 1: Bitcode Parser (2-3 days)
**Goal**: Parse .bc files into simplified IR

**Tasks**:
1. Study LLVM bitcode format (bitstream wrapper)
2. Implement bitstream reader
3. Parse module header, function definitions
4. Extract basic blocks and instructions
5. Create simplified IR representation
6. Test with fibonacci.bc, sum.bc, compute.bc

**Files**:
- `kernel/bitcode_parser.h` (types and API)
- `kernel/bitcode_parser.c` (implementation)
- `kernel/bitcode_test.c` (unit tests)

**Validation**:
- Successfully parse embedded test bitcode
- Print IR in human-readable format
- Verify instruction opcodes match LLVM spec

### Phase 2: IR Interpreter (3-4 days)
**Goal**: Execute parsed IR functions

**Tasks**:
1. Implement virtual register file
2. Implement stack machine (alloca, load, store)
3. Implement arithmetic instructions
4. Implement control flow (br, ret, call)
5. Implement comparison and conversion ops
6. Test with simple functions

**Files**:
- `kernel/ir_interpreter.h` (execution context, API)
- `kernel/ir_interpreter.c` (instruction handlers)
- `kernel/ir_test.c` (tests)

**Validation**:
- fibonacci(5) returns 5
- sum(1..100) returns 5050
- Nested loops execute correctly

### Phase 3: Pattern Detection & Micro-JIT Bridge (2-3 days)
**Goal**: Connect interpreter with existing Micro-JIT

**Tasks**:
1. Implement CFG analysis for pattern detection
2. Add fibonacci pattern detector
3. Add sum pattern detector
4. Add simple loop pattern detector
5. Bridge to micro_jit_compile_*() functions
6. Add fallback to interpreter for unknown patterns

**Files**:
- `kernel/pattern_jit_bridge.h`
- `kernel/pattern_jit_bridge.c`

**Validation**:
- Detect fibonacci pattern from IR
- Compile to x86 with Micro-JIT
- Execute compiled code successfully
- Fallback to interpreter for unknown code

### Phase 4: Adaptive JIT Integration (2 days)
**Goal**: Full workflow with progressive optimization

**Tasks**:
1. Modify adaptive_jit to support bitcode modules
2. Add call counting and profiling
3. Implement O0 (interpret) → O1 (Micro-JIT) transitions
4. Test atomic code swapping with interpreted→compiled
5. Add performance benchmarking

**Files**:
- `kernel/adaptive_jit.c` (modify existing)
- `kernel/adaptive_jit.h` (extend API)

**Validation**:
- Load fibonacci.bc from disk
- Execute interpreted (slow)
- After 100 calls, recompile with Micro-JIT
- After 1000 calls, apply optimizations
- Verify atomic swap works
- Measure speedup: interpreted vs compiled

### Phase 5: End-to-End Demo (1 day)
**Goal**: Demonstrate complete workflow

**Tasks**:
1. Create demo module (e.g., mandelbrot.bc)
2. Load from FAT16 disk
3. Profile execution
4. Show progressive optimization
5. Export profiling data

**Files**:
- `kernel/jit_demo.c`

**Demo Output**:
```
Loading mandelbrot.bc from disk...
Parsing bitcode... OK
Detected pattern: LOOP_SIMPLE
Interpreting... (100 iterations)
  Average: 50000 cycles/call
Hot path detected! Compiling with Micro-JIT...
  Average: 5000 cycles/call (10x speedup)
Very hot path! Applying optimizations...
  Average: 2500 cycles/call (20x speedup)
```

---

## Size Estimates

| Component | Estimated Size |
|-----------|----------------|
| Bitcode Parser | 15 KB |
| IR Interpreter | 20 KB |
| Pattern JIT Bridge | 10 KB |
| Adaptive JIT Extension | 5 KB |
| **Total** | **50 KB** |

**With current kernel (57KB)**: 57 + 50 = **107 KB** (still under 128 sector limit ✅)

---

## Performance Expectations

### Interpreted Code
- **Overhead**: 10-100x slower than native
- **Use case**: Cold paths, initialization, rare functions
- **Example**: fibonacci(5) = 150K cycles interpreted vs 15K compiled

### Pattern-Matched + Micro-JIT
- **Overhead**: 1-2x slower than -O2
- **Use case**: Hot loops, common patterns
- **Example**: fibonacci(5) = 15K-20K cycles

### Aggressive Optimizations (Future)
- **Overhead**: Near -O2 performance
- **Use case**: Very hot paths (10K+ calls)
- **Example**: fibonacci(5) = 8K-12K cycles

---

## Alternative: Wait for LLVM Minimal Port

If IR interpreter proves too complex, alternative path:

1. **Static link minimal LLVM subset**
   - Only x86 target (not all backends)
   - Only essential optimization passes
   - Strip debug info and metadata
   - Estimated: 5-10MB compressed

2. **Load LLVM as kernel module**
   - Boot minimal kernel (current 57KB)
   - Load LLVM.ko from disk as "module"
   - Use FAT16 to read 10MB file
   - Map to high memory (0x1000000+)
   - Jump to LLVM JIT functions

This approach requires:
- Custom ELF loader for bare-metal
- Symbol resolution without libc
- Memory management for large blobs
- More complex, but gets full LLVM power

**Verdict**: Keep as backup plan, proceed with IR interpreter first.

---

## Decision

✅ **Implement Hybrid Micro-JIT + IR Interpreter**

**Rationale**:
1. Fits in current kernel size constraints
2. Aligns with user priority (runtime optimization)
3. Leverages existing Micro-JIT infrastructure
4. Provides upgrade path (interpreted → compiled)
5. Pragmatic: working solution in 1-2 weeks

**Next Steps**:
1. Start with Phase 1: Bitcode Parser
2. Validate with existing test modules
3. Iterate based on performance measurements

---

**Status**: PLAN APPROVED
**Estimated Duration**: 10-14 days
**Risk Level**: MEDIUM (bitcode format complexity)
**Fallback**: Expand bootloader + static link LLVM (if interpreter fails)
