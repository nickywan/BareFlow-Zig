#!/bin/bash
# run.sh - Build and run Fluid kernel

set -e

echo "Building Fluid kernel..."
make clean
make

echo ""
echo "Launching QEMU..."
echo "Press Ctrl+C to exit"
make run