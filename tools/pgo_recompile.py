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
from typing import Dict, List, Tuple


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

    if args.plan_out:
        plan = {
            "profile_source": str(args.profile),
            "generated_from": data.get("hostname"),
            "generated_at": data.get("generated_at"),
            "profile_tag": data.get("profile_tag"),
            "call_threshold": args.call_threshold,
            "cycle_threshold": args.cycle_threshold,
            "modules": plan_modules,
        }
        args.plan_out.parent.mkdir(parents=True, exist_ok=True)
        with args.plan_out.open("w", encoding="utf-8") as fh:
            json.dump(plan, fh, indent=2)
        print(f"\nWrote plan to {args.plan_out}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
