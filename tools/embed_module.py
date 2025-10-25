#!/usr/bin/env python3
"""Embed a raw module binary as a C array."""
from __future__ import annotations

import argparse
import pathlib
import textwrap


def sanitize(name: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in name)


def main() -> int:
    parser = argparse.ArgumentParser(description="Embed module binary into C source")
    parser.add_argument("--input", required=True, type=pathlib.Path)
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--name", required=True, help="Module identifier (e.g. compute)")
    args = parser.parse_args()

    data = args.input.read_bytes()
    ident = sanitize(args.name)

    array_name = f"cache_module_{ident}"
    lines = [
        "#include <stdint.h>\n",
        f"const unsigned char {array_name}[{len(data)}] __attribute__((aligned(16))) = {{\n",
    ]

    row = []
    for idx, byte in enumerate(data):
        row.append(f"0x{byte:02x}")
        if len(row) == 12 or idx == len(data) - 1:
            lines.append("    " + ", ".join(row) + ("\n" if idx == len(data) - 1 else ",\n"))
            row = []

    lines.append("};\n")
    lines.append(f"const unsigned int cache_module_{ident}_size = {len(data)};\n")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
