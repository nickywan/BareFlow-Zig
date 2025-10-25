#!/usr/bin/env python3
"""
Bitcode Module Wrapper - Create .bc modules with header

Takes LLVM bitcode (.bc) and wraps it with BareFlow header.

Usage:
    python3 tools/wrap_bitcode.py --input module.bc --output module_wrapped.bc \
        --name fibonacci --entry module_fibonacci

Output format:
    [bitcode_header_t: 128 bytes]
    [LLVM bitcode: variable]
"""

import argparse
import struct
import sys
from pathlib import Path

BITCODE_MAGIC = 0x4C4C4243  # "LLBC"
HEADER_SIZE = 128  # sizeof(bitcode_header_t)

def create_header(module_name, entry_name, bitcode_size, opt_level=0, version=1):
    """Create bitcode module header (128 bytes)"""
    # Encode strings
    name_bytes = module_name.encode('utf-8')[:32].ljust(32, b'\x00')
    entry_bytes = entry_name.encode('utf-8')[:64].ljust(64, b'\x00')

    # Pack header
    header = struct.pack(
        '<I32s64sIIII8s',  # little-endian
        BITCODE_MAGIC,     # magic
        name_bytes,        # module_name[32]
        entry_bytes,       # entry_name[64]
        bitcode_size,      # bitcode_size
        version,           # version
        opt_level,         # opt_level
        0,                 # reserved[0]
        b'\x00' * 8        # reserved[1] + padding
    )

    return header

def wrap_bitcode(input_path, output_path, module_name, entry_name, opt_level=0):
    """Wrap LLVM bitcode with BareFlow header"""
    # Read bitcode
    with open(input_path, 'rb') as f:
        bitcode_data = f.read()

    bitcode_size = len(bitcode_data)

    print(f"📦 Wrapping bitcode module:")
    print(f"   Input: {input_path}")
    print(f"   Module name: {module_name}")
    print(f"   Entry: {entry_name}")
    print(f"   Bitcode size: {bitcode_size} bytes")
    print(f"   Opt level: O{opt_level}")

    # Create header
    header = create_header(module_name, entry_name, bitcode_size, opt_level)

    # Write wrapped module
    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(bitcode_data)

    total_size = len(header) + bitcode_size
    print(f"   ✓ Written: {output_path} ({total_size} bytes)")

    return 0

def main():
    parser = argparse.ArgumentParser(description="Wrap LLVM bitcode with BareFlow header")
    parser.add_argument("--input", required=True, help="Input .bc file")
    parser.add_argument("--output", required=True, help="Output wrapped .bc file")
    parser.add_argument("--name", required=True, help="Module name (max 32 chars)")
    parser.add_argument("--entry", required=True, help="Entry function name (max 64 chars)")
    parser.add_argument("--opt", type=int, default=0, choices=[0,1,2,3],
                        help="Optimization level (0-3)")
    args = parser.parse_args()

    # Validate inputs
    if not Path(args.input).exists():
        print(f"❌ Input file not found: {args.input}")
        return 1

    if len(args.name) > 32:
        print(f"❌ Module name too long (max 32): {args.name}")
        return 1

    if len(args.entry) > 64:
        print(f"❌ Entry name too long (max 64): {args.entry}")
        return 1

    return wrap_bitcode(args.input, args.output, args.name, args.entry, args.opt)

if __name__ == "__main__":
    sys.exit(main())
