#!/bin/bash
# ============================================================================
# Compile LLVM Module to ELF at Multiple Optimization Levels
# ============================================================================
# Usage: ./compile_llvm_module.sh <source.c> <output_basename>
# Produces: <output_basename>_O0.elf, _O1.elf, _O2.elf, _O3.elf

set -e

if [ $# -ne 2 ]; then
    echo "Usage: $0 <source.c> <output_basename>"
    echo "Example: $0 fibonacci.c fibonacci"
    exit 1
fi

SOURCE=$1
BASENAME=$2
OUTPUT_DIR="llvm_modules"

mkdir -p "$OUTPUT_DIR"

echo "=== Compiling LLVM Module: $SOURCE ==="
echo ""

# Compile to bitcode first
echo "[1/5] Compiling to LLVM bitcode..."
clang-18 -m32 -ffreestanding -nostdlib -emit-llvm -c "$SOURCE" -o "${OUTPUT_DIR}/${BASENAME}.bc"
echo "      ✓ ${BASENAME}.bc created"

# Compile to ELF at different optimization levels
for OPT_LEVEL in O0 O1 O2 O3; do
    echo ""
    echo "[$(echo $OPT_LEVEL | tr -d 'O' | awk '{print $1+2}')/5] Compiling with -${OPT_LEVEL}..."

    # Compile bitcode to object file
    clang-18 -m32 -ffreestanding -nostdlib -${OPT_LEVEL} -c "${OUTPUT_DIR}/${BASENAME}.bc" \
        -o "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.o"

    # Link to ELF executable
    ld -m elf_i386 -e compute "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.o" \
        -o "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.elf"

    # Get file size
    SIZE=$(stat -c%s "${OUTPUT_DIR}/${BASENAME}_${OPT_LEVEL}.elf")

    echo "      ✓ ${BASENAME}_${OPT_LEVEL}.elf created (${SIZE} bytes)"
done

echo ""
echo "=== Compilation Complete ==="
echo ""
echo "Output files in ${OUTPUT_DIR}:"
ls -lh "${OUTPUT_DIR}/${BASENAME}"*.elf | awk '{print "  " $9 " - " $5}'

echo ""
echo "Bitcode file:"
ls -lh "${OUTPUT_DIR}/${BASENAME}.bc" | awk '{print "  " $9 " - " $5}'
