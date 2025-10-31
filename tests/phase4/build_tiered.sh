#!/bin/bash
# Build script for test_tiered_llvm

cd "$(dirname "$0")"

echo "Building test_tiered_llvm..."
clang++-18 -g test_tiered_llvm.cpp -o test_tiered_llvm \
  $(llvm-config-18 --cxxflags --ldflags --system-libs --libs core orcjit native passes)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Run with: ./test_tiered_llvm"
else
    echo "Build failed!"
    exit 1
fi
