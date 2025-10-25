#!/bin/bash
# ============================================================================
# BareFlow - Extract PGO Profile from QEMU Serial Output
# ============================================================================
# Extracts profile data from kernel serial output for PGO recompilation
#
# Usage: ./extract_pgo_profile.sh <serial_log.txt> <output_profile.txt>
#

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <serial_log.txt> <output_profile.txt>"
    echo "Example: $0 /tmp/serial.log profile_data.txt"
    exit 1
fi

INPUT=$1
OUTPUT=$2

echo "Extracting PGO profile data from: $INPUT"

# Extract profile data section from serial log
# Profile data is between "=== LLVM PGO PROFILE EXPORT ===" markers

awk '
/=== LLVM PGO PROFILE EXPORT ===/ { capture=1; next }
/=== END OF PROFILE DATA ===/ { capture=0; next }
capture { print }
' "$INPUT" > "$OUTPUT"

if [ -s "$OUTPUT" ]; then
    echo "Profile data extracted to: $OUTPUT"
    echo ""
    echo "Summary:"
    grep "^# Total Modules:" "$OUTPUT" || echo "# Total Modules: (not found)"
    echo ""
    grep -E "^[^#].*:compute:" "$OUTPUT" | while IFS=: read -r module func calls cycles; do
        echo "  - $module: $calls calls, $cycles total cycles"
    done
    echo ""
    echo "Profile data ready for PGO compilation!"
    echo "Use: ./tools/compile_llvm_pgo.sh <source.c> <module> $OUTPUT"
else
    echo "Error: No profile data found in $INPUT"
    echo "Make sure the kernel ran and exported PGO data to serial output."
    exit 1
fi
