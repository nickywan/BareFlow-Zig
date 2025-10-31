#!/bin/bash
# analyze_llvm_usage.sh - Analyze LLVM library usage
#
# Purpose: Identify which LLVM symbols are used vs total available
# Goal: Measure dead code elimination potential

echo "=== LLVM Usage Analysis ==="
echo ""

# Binary to analyze
BINARY="test_tiered_jit"
LLVM_LIB="/lib/x86_64-linux-gnu/libLLVM-18.so.1"

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary $BINARY not found"
    echo "Run: make -f Makefile.tiered first"
    exit 1
fi

echo "[1] Binary Information"
echo "----------------------"
ls -lh $BINARY
echo ""

echo "[2] LLVM Library Information"
echo "----------------------------"
ls -lh $LLVM_LIB
echo ""

echo "[3] Symbol Counts"
echo "-----------------"

# Count symbols
TOTAL_LLVM_SYMBOLS=$(nm -D $LLVM_LIB 2>/dev/null | grep " T " | wc -l)
USED_LLVM_SYMBOLS=$(nm -D $BINARY | grep " U " | grep -i llvm | wc -l)

echo "Total LLVM exported functions: $TOTAL_LLVM_SYMBOLS"
echo "Used by $BINARY: $USED_LLVM_SYMBOLS"
echo ""

# Calculate percentage
USAGE_PERCENT=$(echo "scale=4; $USED_LLVM_SYMBOLS * 100 / $TOTAL_LLVM_SYMBOLS" | bc)
UNUSED_PERCENT=$(echo "scale=2; 100 - $USAGE_PERCENT" | bc)

echo "Usage: $USAGE_PERCENT%"
echo "Dead code (unused): $UNUSED_PERCENT%"
echo ""

echo "[4] Used LLVM Symbols (first 50)"
echo "---------------------------------"
nm -D $BINARY | grep " U " | grep -i llvm | head -50
echo ""
echo "... (total: $USED_LLVM_SYMBOLS symbols)"
echo ""

echo "[5] Size Analysis"
echo "-----------------"

LLVM_SIZE_MB=$(ls -lh $LLVM_LIB | awk '{print $5}')
BINARY_SIZE_KB=$(ls -lh $BINARY | awk '{print $5}')

echo "libLLVM-18.so.1: $LLVM_SIZE_MB"
echo "test_tiered_jit: $BINARY_SIZE_KB"
echo ""

echo "[6] Theoretical Size Reduction"
echo "-------------------------------"

# Estimate minimal LLVM size based on usage
LLVM_SIZE_BYTES=$(stat -c%s $LLVM_LIB)
ESTIMATED_MINIMAL=$(echo "scale=0; $LLVM_SIZE_BYTES * $USED_LLVM_SYMBOLS / $TOTAL_LLVM_SYMBOLS / 1024 / 1024" | bc)

echo "Current LLVM size: 118 MB"
echo "Estimated minimal size (if only used symbols): ~$ESTIMATED_MINIMAL MB"
echo "Potential reduction: ~$(echo "118 - $ESTIMATED_MINIMAL" | bc) MB"
echo ""

echo "[7] Dependencies"
echo "----------------"
ldd $BINARY | grep -i llvm
echo ""

echo "=== Analysis Complete ==="
echo ""
echo "Summary:"
echo "--------"
echo "- Only $USAGE_PERCENT% of LLVM is used"
echo "- Dead code elimination potential: $UNUSED_PERCENT%"
echo "- Estimated size reduction: ~$(echo "118 - $ESTIMATED_MINIMAL" | bc) MB"
echo ""
echo "Next steps:"
echo "1. Custom LLVM build with only used components"
echo "2. Static linking with -Wl,--gc-sections"
echo "3. Strip unused symbols"
