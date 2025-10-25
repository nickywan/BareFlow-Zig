#!/bin/bash
# ============================================================================
# PGO Performance Benchmark Script
# ============================================================================
# Compares execution performance between standard and PGO-optimized modules
#
# Usage: ./tools/benchmark_pgo.sh [module_name]
#
# Example:
#   ./tools/benchmark_pgo.sh matrix_mul
#   ./tools/benchmark_pgo.sh all
# ============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to extract cycle count from a module's profile
extract_cycles() {
    local module=$1
    local profile=$2

    # Extract avg cycles/call from profile
    grep -A 5 "# Module: $module" "$profile" | grep "# Avg Cycles/Call:" | awk '{print $5}'
}

# Function to compare two cycle counts and calculate speedup
calculate_speedup() {
    local baseline=$1
    local optimized=$2

    # Use bc for floating point calculation
    echo "scale=2; $baseline / $optimized" | bc
}

# Function to benchmark a single module
benchmark_module() {
    local module=$1
    local source=$2

    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Benchmarking: $module${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # Check if module source exists
    if [ ! -f "$source" ]; then
        echo -e "${RED}Error: Source file $source not found${NC}"
        return 1
    fi

    # Check if standard binaries exist
    if [ ! -f "llvm_modules/${module}_O0.elf" ]; then
        echo -e "${YELLOW}Standard binaries not found, compiling...${NC}"
        ./tools/compile_llvm_module.sh "$source" "$module"
    fi

    # Check if PGO binaries exist
    if [ ! -f "llvm_modules/${module}_O0_pgo.elf" ]; then
        echo -e "${YELLOW}PGO binaries not found, need profile data${NC}"
        echo -e "${YELLOW}Run kernel first to generate profile data${NC}"
        return 1
    fi

    # Compare binary sizes
    echo -e "${YELLOW}[1] Binary Size Comparison${NC}"
    echo ""
    echo "Optimization Level | Standard | PGO | Difference"
    echo "-------------------|----------|-----|------------"

    for level in O0 O1 O2 O3; do
        std_size=$(stat -c%s "llvm_modules/${module}_${level}.elf" 2>/dev/null || echo "0")
        pgo_size=$(stat -c%s "llvm_modules/${module}_${level}_pgo.elf" 2>/dev/null || echo "0")
        diff=$((pgo_size - std_size))

        printf "%-18s | %8d | %7d | %+7d bytes\n" "$level" "$std_size" "$pgo_size" "$diff"
    done

    echo ""

    # Compare disassembly (instruction count)
    echo -e "${YELLOW}[2] Code Analysis${NC}"
    echo ""

    # Generate disassembly for standard O3 if not exists
    if [ ! -f "llvm_modules/${module}_O3.asm" ]; then
        objdump -d -M intel "llvm_modules/${module}_O3.elf" > "llvm_modules/${module}_O3.asm"
    fi

    # Count instructions in compute function
    std_instructions=$(grep -A 100 "<compute>:" "llvm_modules/${module}_O3.asm" 2>/dev/null | grep -E "^\s+[0-9a-f]+:" | wc -l || echo "0")
    pgo_instructions=$(grep -A 100 "<compute>:" "llvm_modules/${module}_O3_pgo.asm" 2>/dev/null | grep -E "^\s+[0-9a-f]+:" | wc -l || echo "0")

    echo "Standard O3 instructions: $std_instructions"
    echo "PGO O3 instructions:      $pgo_instructions"

    if [ "$pgo_instructions" -lt "$std_instructions" ]; then
        reduction=$((std_instructions - pgo_instructions))
        percent=$(echo "scale=1; $reduction * 100 / $std_instructions" | bc)
        echo -e "${GREEN}→ PGO reduced instruction count by $reduction ($percent%)${NC}"
    elif [ "$pgo_instructions" -gt "$std_instructions" ]; then
        increase=$((pgo_instructions - std_instructions))
        percent=$(echo "scale=1; $increase * 100 / $std_instructions" | bc)
        echo -e "${YELLOW}→ PGO increased instruction count by $increase ($percent%) - may include loop unrolling${NC}"
    else
        echo "→ No change in instruction count"
    fi

    echo ""

    # Check for specific optimizations in disassembly
    echo -e "${YELLOW}[3] Optimization Techniques Detected${NC}"
    echo ""

    # Check for loop unrolling (multiple similar instruction sequences)
    unroll_std=$(grep -E "imul|add.*," "llvm_modules/${module}_O3.asm" 2>/dev/null | wc -l || echo "0")
    unroll_pgo=$(grep -E "imul|add.*," "llvm_modules/${module}_O3_pgo.asm" 2>/dev/null | wc -l || echo "0")

    if [ "$unroll_pgo" -gt 0 ] && [ "$unroll_std" -gt 0 ]; then
        threshold=$((unroll_std + unroll_std / 4))
        if [ "$unroll_pgo" -gt "$threshold" ]; then
            echo -e "${GREEN}✓ Loop unrolling detected (more multiply/add instructions)${NC}"
        fi
    fi

    # Check for vectorization (SIMD instructions)
    simd_std=$(grep -E "xmm|ymm|zmm" "llvm_modules/${module}_O3.asm" 2>/dev/null | wc -l || echo "0")
    simd_pgo=$(grep -E "xmm|ymm|zmm" "llvm_modules/${module}_O3_pgo.asm" 2>/dev/null | wc -l || echo "0")

    if [ "$simd_pgo" -gt "$simd_std" ]; then
        echo -e "${GREEN}✓ Vectorization detected (SIMD instructions added)${NC}"
    fi

    # Check for function inlining (fewer call instructions)
    calls_std=$(grep -c "call" "llvm_modules/${module}_O3.asm" 2>/dev/null || echo "0")
    calls_pgo=$(grep -c "call" "llvm_modules/${module}_O3_pgo.asm" 2>/dev/null || echo "0")

    if [ "$calls_pgo" -lt "$calls_std" ]; then
        reduced=$((calls_std - calls_pgo))
        echo -e "${GREEN}✓ Function inlining detected ($reduced fewer call instructions)${NC}"
    fi

    # Check for constant propagation (more immediate values)
    imm_std=$(grep -E "\\\$0x" "llvm_modules/${module}_O3.asm" 2>/dev/null | wc -l || echo "0")
    imm_pgo=$(grep -E "\\\$0x" "llvm_modules/${module}_O3_pgo.asm" 2>/dev/null | wc -l || echo "0")

    if [ "$imm_pgo" -gt 0 ] && [ "$imm_std" -gt 0 ]; then
        threshold=$((imm_std + 5))
        if [ "$imm_pgo" -gt "$threshold" ]; then
            echo -e "${GREEN}✓ Constant propagation detected (more immediate values)${NC}"
        fi
    fi

    echo ""

    # Extract cycle information from profile if available
    if [ -f "profile_all_modules.txt" ]; then
        echo -e "${YELLOW}[4] Runtime Performance (from profile)${NC}"
        echo ""

        avg_cycles=$(extract_cycles "$module" "profile_all_modules.txt")

        if [ -n "$avg_cycles" ] && [ "$avg_cycles" != "" ]; then
            echo "Baseline avg cycles/call: $avg_cycles"
            echo ""
            echo -e "${BLUE}Note: To measure PGO speedup, need to run both versions${NC}"
            echo -e "${BLUE}and compare execution times. This requires kernel rebuild.${NC}"
        fi
    fi

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Benchmark Complete: $module${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
}

# Main script
main() {
    local module=$1

    echo ""
    echo -e "${BLUE}============================================${NC}"
    echo -e "${BLUE}  PGO Performance Benchmark Tool${NC}"
    echo -e "${BLUE}============================================${NC}"
    echo ""

    if [ "$module" = "all" ] || [ -z "$module" ]; then
        echo "Benchmarking all modules..."
        echo ""

        benchmark_module "matrix_mul" "llvm_modules/matrix_mul.c"
        benchmark_module "sha256" "llvm_modules/sha256.c"
        benchmark_module "primes" "llvm_modules/prime_sieve.c"

        echo ""
        echo -e "${GREEN}All benchmarks complete!${NC}"
        echo ""
        echo -e "${YELLOW}Summary:${NC}"
        echo "Standard binaries: llvm_modules/*_O[0-3].elf"
        echo "PGO binaries:      llvm_modules/*_O[0-3]_pgo.elf"
        echo "Disassembly:       llvm_modules/*_O3_pgo.asm"
        echo ""
        echo -e "${BLUE}To measure real speedup:${NC}"
        echo "1. Replace standard binaries with PGO versions"
        echo "2. Rebuild kernel"
        echo "3. Run and compare profile data"

    elif [ "$module" = "matrix_mul" ]; then
        benchmark_module "matrix_mul" "llvm_modules/matrix_mul.c"
    elif [ "$module" = "sha256" ]; then
        benchmark_module "sha256" "llvm_modules/sha256.c"
    elif [ "$module" = "primes" ]; then
        benchmark_module "primes" "llvm_modules/prime_sieve.c"
    else
        echo -e "${RED}Error: Unknown module '$module'${NC}"
        echo ""
        echo "Usage: $0 [module_name]"
        echo ""
        echo "Available modules:"
        echo "  matrix_mul"
        echo "  sha256"
        echo "  primes"
        echo "  all (benchmark all modules)"
        exit 1
    fi
}

main "$@"
