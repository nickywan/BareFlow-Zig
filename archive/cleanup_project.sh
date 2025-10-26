#!/bin/bash

# BareFlow Project Cleanup Script
# Date: 2025-10-26
# Purpose: Organize project structure after Phase 3 completion

echo "🧹 BareFlow Project Cleanup"
echo "============================"
echo ""

# Function to ask for confirmation
confirm() {
    read -p "$1 (y/n)? " -n 1 -r
    echo
    [[ $REPLY =~ ^[Yy]$ ]]
}

# 1. Archive old monolithic kernel code
if [ -d "kernel" ] && confirm "Archive old kernel/ directory to archive/monolithic_kernel"; then
    echo "📁 Archiving old kernel code..."
    mkdir -p archive/monolithic_kernel
    mv kernel archive/monolithic_kernel/ 2>/dev/null && echo "  ✓ kernel/ archived"
    [ -d "modules" ] && mv modules archive/monolithic_kernel/ 2>/dev/null && echo "  ✓ modules/ archived"
fi

# 2. Organize Phase 3 tests
if confirm "Move test programs to tests/phase3/"; then
    echo "🧪 Organizing test programs..."
    mkdir -p tests/phase3

    # Move test source files
    for file in test_*.cpp test_*.c; do
        [ -f "$file" ] && mv "$file" tests/phase3/ && echo "  ✓ Moved $file"
    done

    # Move Makefiles for tests
    [ -f "Makefile.tiered" ] && mv Makefile.tiered tests/phase3/
    [ -f "Makefile.interpreter" ] && mv Makefile.interpreter tests/phase3/
    [ -f "Makefile.jit_test" ] && mv Makefile.jit_test tests/phase3/
fi

# 3. Organize Phase 3 documentation
if confirm "Move Phase 3 docs to docs/phase3/"; then
    echo "📚 Organizing documentation..."
    mkdir -p docs/phase3

    for file in PHASE3_*.md; do
        [ -f "$file" ] && mv "$file" docs/phase3/ && echo "  ✓ Moved $file"
    done
fi

# 4. Clean test binaries
if confirm "Remove test binary executables"; then
    echo "🗑️  Cleaning test binaries..."
    rm -f test_full_static test_jit_minimal test_llvm_interpreter 2>/dev/null
    rm -f test_tiered_jit test_matrix_jit test_native_export 2>/dev/null
    rm -f test_matmul_O0 test_matmul_O2 test_matmul_O3 2>/dev/null
    rm -f test_jit_diagnostic test_jit_fib20 test_jit_sum 2>/dev/null
    rm -f test_micro_jit test_micro_jit_fixed test_simple_jit 2>/dev/null
    rm -f test_fib_debug 2>/dev/null
    echo "  ✓ Test binaries removed"
fi

# 5. Clean object files
if confirm "Remove all .o files"; then
    echo "🧹 Cleaning object files..."
    find . -name "*.o" -type f -delete
    echo "  ✓ Object files removed"
fi

# 6. Create .gitignore if needed
if [ ! -f ".gitignore" ] || confirm "Update .gitignore"; then
    echo "📝 Updating .gitignore..."
    cat >> .gitignore << 'EOF'

# Test binaries
tests/phase3/test_*
!tests/phase3/*.cpp
!tests/phase3/*.c

# Object files
*.o
*.a

# Build artifacts
build/*.bin
build/*.img
build/*.elf

# Generated images
*.img
!tinyllama/tinyllama.img

# IDE
.vscode/
.idea/

# Temporary files
*.tmp
*.swp
*~
EOF
    echo "  ✓ .gitignore updated"
fi

echo ""
echo "✅ Cleanup complete!"
echo ""
echo "📊 Project Structure:"
echo "===================="
tree -d -L 2 2>/dev/null || ls -la

echo ""
echo "💡 Next Steps:"
echo "1. Review the changes"
echo "2. Run: git status"
echo "3. Commit with: git add -A && git commit -m 'refactor: Organize project structure after Phase 3'"
echo "4. Start Phase 4: Custom LLVM build for bare-metal"