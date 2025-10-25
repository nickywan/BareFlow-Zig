#!/usr/bin/env python3
"""Generate cache registry source listing embedded modules."""
from __future__ import annotations

import argparse
import pathlib


def sanitize(name: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in name)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate cache registry source")
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--modules", nargs="+", help="Module base names")
    args = parser.parse_args()

    modules = sorted(set(args.modules))
    lines = [
        "#include \"cache_loader.h\"\n",
        "\n",
    ]

    for name in modules:
        ident = sanitize(name)
        lines.append(f"extern const unsigned char cache_module_{ident}[];\n")
        lines.append(f"extern const unsigned int cache_module_{ident}_size;\n")

    lines.append("\n")
    lines.append("void cache_registry_foreach(cache_registry_iter_fn fn, void* ctx) {\n")
    lines.append("    if (!fn) {\n")
    lines.append("        return;\n")
    lines.append("    }\n")
    for name in modules:
        ident = sanitize(name)
        lines.append(
            f"    fn(\"{name}\", cache_module_{ident}, cache_module_{ident}_size, ctx);\n"
        )
    lines.append("}\n")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
