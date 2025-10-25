#!/usr/bin/env python3
"""
Analyze profiling export JSON and recommend recompilation actions.

This script is a first step toward the offline PGO workflow described in the roadmap.
It parses the kernel's JSON export, identifies hot modules, and emits a plan
indicating which modules should be recompiled with higher optimization levels.

Usage:
    python3 tools/pgo_recompile.py export.json --plan-out build/pgo_plan.json
"""
from __future__ import annotations

import argparse
import json
import pathlib
import sys
import shutil
import subprocess
from typing import Dict, List, Optional, Tuple


def load_profile(path: pathlib.Path) -> Dict[str, object]:
    with path.open(encoding="utf-8") as fh:
        return json.load(fh)


def rank_modules(mods: List[Dict[str, object]], sort_key: str) -> List[Dict[str, object]]:
    return sorted(mods, key=lambda m: m.get(sort_key, 0), reverse=True)


def recommend_level(module: Dict[str, object], call_threshold: int, cycle_threshold: int) -> Tuple[str, str]:
    calls = int(module.get("calls", 0))
    total_cycles = int(module.get("total_cycles", 0))
    if calls >= call_threshold * 10 or total_cycles >= cycle_threshold * 10:
        return "O3", "ultra-hot (>=10x threshold)"
    if calls >= call_threshold or total_cycles >= cycle_threshold:
        return "O2", "hot (>= threshold)"
    if calls > 0:
        return "O1", "warm (executed but below threshold)"
    return "O0", "cold (never executed)"


def main() -> int:
    parser = argparse.ArgumentParser(description="Offline PGO helper")
    parser.add_argument("profile", type=pathlib.Path, help="JSON export from kernel profiling")
    parser.add_argument(
        "--call-threshold",
        type=int,
        default=100,
        help="Calls threshold for hot classification (default: 100)",
    )
    parser.add_argument(
        "--cycle-threshold",
        type=int,
        default=100_000,
        help="Total cycles threshold for hot classification (default: 100000)",
    )
    parser.add_argument(
        "--plan-out",
        type=pathlib.Path,
        help="Optional path to write JSON plan",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Recompile modules according to suggested optimization levels",
    )
    parser.add_argument(
        "--module-dir",
        type=pathlib.Path,
        default=pathlib.Path("modules"),
        help="Directory containing module source files (default: modules/)",
    )
    parser.add_argument(
        "--output-dir",
        type=pathlib.Path,
        default=pathlib.Path("cache"),
        help="Directory to place recompiled modules when using --apply (default: cache/)",
    )
    parser.add_argument(
        "--profile-tag",
        type=str,
        help="Profile tag to use for cache output (defaults to value in profile JSON or 'default')",
    )
    args = parser.parse_args()

    data = load_profile(args.profile)
    modules = data.get("modules")
    if not isinstance(modules, list):
        print("Profile JSON is missing 'modules' array", file=sys.stderr)
        return 1

    ranked = rank_modules(modules, "total_cycles")
    plan_modules = []

    print(f"Profile tag: {data.get('profile_tag', 'unknown')}")
    print(f"Total modules: {len(ranked)}")
    print("Recommended recompile plan:\n")
    print(f"{'Module':<16} {'Calls':>10} {'Cycles':>15} {'Suggested':>10}  Reason")
    print("-" * 64)
    for mod in ranked:
        name = mod.get("name", "<unnamed>")
        calls = int(mod.get("calls", 0))
        cycles = int(mod.get("total_cycles", 0))
        level, reason = recommend_level(mod, args.call_threshold, args.cycle_threshold)
        print(f"{name:<16} {calls:>10} {cycles:>15} {level:>10}  {reason}")

        plan_modules.append(
            {
                "name": name,
                "calls": calls,
                "total_cycles": cycles,
                "suggested_opt": level,
                "reason": reason,
            }
        )

    plan = {
        "profile_source": str(args.profile),
        "generated_from": data.get("hostname"),
        "generated_at": data.get("generated_at"),
        "profile_tag": data.get("profile_tag"),
        "call_threshold": args.call_threshold,
        "cycle_threshold": args.cycle_threshold,
        "modules": plan_modules,
    }
    if args.plan_out:
        args.plan_out.parent.mkdir(parents=True, exist_ok=True)
        with args.plan_out.open("w", encoding="utf-8") as fh:
            json.dump(plan, fh, indent=2)
        print(f"\nWrote plan to {args.plan_out}")

    if args.apply:
        apply_recompile(plan, args.module_dir, args.output_dir, args.profile_tag)

    return 0


def apply_recompile(plan: Dict[str, object], module_dir: pathlib.Path, output_dir: pathlib.Path, profile_tag_override: Optional[str]) -> None:
    compiler = detect_compiler()
    objcopy = detect_objcopy()
    profile_tag = profile_tag_override or plan.get("profile_tag") or "default"
    print(f"\nApplying plan using compiler '{compiler}' and objcopy '{objcopy}'")
    print(f"Output cache: {output_dir}/ {profile_tag}")

    target_dir = output_dir / str(profile_tag)
    target_dir.mkdir(parents=True, exist_ok=True)

    base_flags = [
        "-m32",
        "-ffreestanding",
        "-nostdlib",
        "-fno-pie",
        "-fno-stack-protector",
        "-Wall",
        "-Wextra",
    ]

    alias_map = {
        "sum": module_dir / "simple_sum.c",
        "primes": module_dir / "primes.c",
    }

    for mod in plan.get("modules", []):
        name = mod.get("name")
        level = mod.get("suggested_opt", "O0")

        if not name:
            continue
        src = module_dir / f"{name}.c"
        if not src.exists():
            src = alias_map.get(name, src)
        if not src.exists():
            print(f"[WARN] Source not found for module '{name}' ({src})")
            continue

        opt_flag = f"-{level.lower()}"
        if level == "O0":
            # still generate baseline
            opt_flag = "-O0"
        elif level in {"O1", "O2", "O3"}:
            opt_flag = f"-{level.lower()}"
        else:
            opt_flag = "-O2"

        print(f"\nCompiling {name} with {opt_flag} ...")
        tmp_o = target_dir / f"{name}_{level}.o"
        tmp_mod = target_dir / f"{name}_{level}.mod"

        cmd = [compiler, *base_flags, opt_flag, "-c", str(src), "-o", str(tmp_o)]
        run_or_fail(cmd, f"compile {name}")

        obj_cmd = [objcopy, "-O", "binary", str(tmp_o), str(tmp_mod)]
        run_or_fail(obj_cmd, f"objcopy {name}")
        tmp_o.unlink(missing_ok=True)

        final_mod = target_dir / f"{name}.mod"
        shutil.copy2(tmp_mod, final_mod)

        print(f"  -> {tmp_mod} ({tmp_mod.stat().st_size} bytes)")
        print(f"  -> active copy: {final_mod}")


def detect_compiler() -> str:
    for candidate in ("clang-18", "clang"):
        if shutil.which(candidate):
            return candidate
    raise RuntimeError("clang compiler not found (expected clang-18 or clang)")


def detect_objcopy() -> str:
    for candidate in ("llvm-objcopy-18", "llvm-objcopy", "objcopy"):
        if shutil.which(candidate):
            return candidate
    raise RuntimeError("objcopy not found (tried llvm-objcopy-18, llvm-objcopy, objcopy)")


def run_or_fail(cmd: List[str], context: str) -> None:
    print("  ", " ".join(cmd))
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        print(proc.stdout)
        print(proc.stderr, file=sys.stderr)
        raise RuntimeError(f"Failed to {context}")



if __name__ == "__main__":
    sys.exit(main())
