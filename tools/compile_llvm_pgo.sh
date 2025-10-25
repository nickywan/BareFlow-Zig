#!/bin/bash
# ============================================================================
# BareFlow - LLVM PGO (Profile-Guided Optimization) Compilation Tool
# ============================================================================
# Recompiles LLVM modules using runtime profile data for optimized code generation
#
# Usage: ./compile_llvm_pgo.sh <source.c> <module_name> <profile_data.txt>
#

set -e

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <source.c> <module_name> [profile_data.txt]"
    echo "Example: $0 llvm_modules/fibonacci.c fibonacci profile_fibonacci.txt"
    exit 1
fi

SOURCE=$1
BASENAME=$2
PROFILE=$3
OUTPUT_DIR="llvm_modules"

echo "========================================"
echo "LLVM PGO Compilation Pipeline"
echo "========================================"
echo "Source: $SOURCE"
echo "Module: $BASENAME"
if [ -n "$PROFILE" ]; then
    echo "Profile: $PROFILE"
else
    echo "Profile: None (standard compilation)"
fi
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Step 1: Generate LLVM bitcode with instrumentation for profiling
echo "[1/5] Generating instrumented bitcode..."
clang-18 -m32 -ffreestanding -nostdlib -emit-llvm \
    -fprofile-generate -c "$SOURCE" \
    -o "${OUTPUT_DIR}/${BASENAME}_instrumented.bc"

# Step 2: Generate non-instrumented bitcode
echo "[2/5] Generating clean bitcode..."
clang-18 -m32 -ffreestanding -nostdlib -emit-llvm \
    -c "$SOURCE" \
    -o "${OUTPUT_DIR}/${BASENAME}.bc"

# Step 3: If profile data exists, use it for PGO compilation
if [ -n "$PROFILE" ] && [ -f "$PROFILE" ]; then
    echo "[3/5] Compiling with PGO (using profile data)..."

    # Parse profile data to create LLVM profdata format
    # Format in profile: module:function:call_count:total_cycles

    # For now, we'll use the profile to determine hotness and compile accordingly
    CALL_COUNT=$(grep "^${BASENAME}:compute:" "$PROFILE" | cut -d: -f3)

    if [ -z "$CALL_COUNT" ]; then
        echo "Warning: No profile data found for $BASENAME, using default optimization"
        CALL_COUNT=0
    fi

    echo "   Profile shows $CALL_COUNT calls"

    # Determine optimization strategy based on call count
    if [ "$CALL_COUNT" -ge 10000 ]; then
        echo "   → VERY HOT: Using O3 with aggressive inlining"
        OPT_FLAGS="-O3 -finline-hint-functions -fvectorize -funroll-loops"
    elif [ "$CALL_COUNT" -ge 1000 ]; then
        echo "   → HOT: Using O2 with moderate inlining"
        OPT_FLAGS="-O2 -finline-hint-functions"
    elif [ "$CALL_COUNT" -ge 100 ]; then
        echo "   → WARM: Using O1 with basic optimization"
        OPT_FLAGS="-O1"
    else
        echo "   → COLD: Using O0 (no optimization)"
        OPT_FLAGS="-O0"
    fi

    # Compile with profile-guided flags
    for OPT_LEVEL in O0 O1 O2 O3; do
        echo "   Compiling ${OPT_LEVEL} (PGO-enhanced)..."

        # Use the profile-determined flags for the highest level
        if [ "$OPT_LEVEL" = "O3" ]; then
            CURRENT_FLAGS="$OPT_FLAGS"
        else
            CURRENT_FLAGS="-${OPT_LEVEL}"
        fi

        clang-18 -m32 -ffreestanding -nostdlib $CURRENT_FLAGS \
            -c "${OUTPUT_DIR}/${BASENAME}.bc" \
            -o "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.o"

        ld -m elf_i386 -e compute "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.o" \
            -o "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.elf"

        SIZE=$(stat -c%s "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.elf")
        echo "      → ${BASENAME}_${OPT_LEVEL}_pgo.elf (${SIZE} bytes)"
    done

else
    echo "[3/5] No profile data - compiling standard optimization levels..."

    # Standard compilation without PGO
    for OPT_LEVEL in O0 O1 O2 O3; do
        echo "   Compiling ${OPT_LEVEL}..."

        clang-18 -m32 -ffreestanding -nostdlib -${OPT_LEVEL} \
            -c "${OUTPUT_DIR}/${BASENAME}.bc" \
            -o "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.o"

        ld -m elf_i386 -e compute "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.o" \
            -o "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.elf"

        SIZE=$(stat -c%s "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.elf")
        echo "      → ${BASENAME}_${OPT_LEVEL}.elf (${SIZE} bytes)"
    done
fi

echo "[4/5] Generating disassembly for analysis..."
for OPT_LEVEL in O0 O1 O2 O3; do
    if [ -f "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.elf" ]; then
        objdump -d "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.elf" \
            > "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}_pgo.asm"
    elif [ -f "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.elf" ]; then
        objdump -d "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.elf" \
            > "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.asm"
    fi
done

echo "[5/5] Generating LLVM IR for comparison..."
llvm-dis-18 "${OUTPUT_DIR}/${BASENAME}.bc" -o "${OUTPUT_DIR}/${BASENAME}.ll"

echo ""
echo "========================================"
echo "Compilation Complete!"
echo "========================================"
echo "Generated files in $OUTPUT_DIR/:"
ls -lh "$OUTPUT_DIR/${BASENAME}"* | tail -n 20
echo ""

if [ -n "$PROFILE" ] && [ -f "$PROFILE" ]; then
    echo "PGO-optimized binaries created with profile guidance."
    echo "Compare standard vs PGO versions to measure improvement."
fi
