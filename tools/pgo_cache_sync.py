#!/usr/bin/env python3
"""
PGO Cache Sync Tool - Write optimized modules to FAT16 disk

Workflow:
1. Read profiling JSON from serial output
2. Classify modules (O1/O2/O3)
3. Recompile hot modules
4. Write .MOD files to FAT16 disk using mtools
5. Kernel loads from disk at next boot

Usage:
    python3 tools/pgo_cache_sync.py profile.json --disk build/fat16_test.img
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description="Sync PGO cache to FAT16 disk")
    parser.add_argument("profile", help="Profile JSON file")
    parser.add_argument("--disk", default="build/fat16_test.img", help="FAT16 disk image")
    parser.add_argument("--cache-dir", default="cache/i686/default", help="Cache directory")
    parser.add_argument("--dry-run", action="store_true", help="Don't actually write to disk")
    args = parser.parse_args()

    # Check inputs
    if not Path(args.profile).exists():
        print(f"❌ Profile not found: {args.profile}")
        return 1

    if not Path(args.disk).exists():
        print(f"❌ Disk image not found: {args.disk}")
        print(f"   Create with: python3 tools/create_fat16_disk.py")
        return 1

    # Load profile
    with open(args.profile) as f:
        profile = json.load(f)

    print(f"📊 Profile: {profile['num_modules']} modules, {profile['total_calls']} calls")
    print()

    # Classify modules (from pgo_recompile.py logic)
    HOT_THRESHOLD = 100000  # cycles
    ULTRA_HOT_CALLS = 10    # call count

    modules = profile['modules']
    to_sync = []

    for mod in modules:
        name = mod['name']
        calls = mod['calls']
        total_cycles = mod['total_cycles']
        avg_cycles = total_cycles / calls if calls > 0 else 0

        # Determine optimization level
        opt_level = None
        if calls >= ULTRA_HOT_CALLS and total_cycles > HOT_THRESHOLD * 10:
            opt_level = "O3"
        elif total_cycles > HOT_THRESHOLD:
            opt_level = "O2"
        elif calls > 0:
            opt_level = "O1"

        if opt_level:
            mod_file = f"{name}.mod"
            opt_file = f"{name}_O{opt_level[-1]}.mod"
            cache_path = Path(args.cache_dir) / opt_file

            if cache_path.exists():
                to_sync.append({
                    "name": name,
                    "opt": opt_level,
                    "file": cache_path,
                    "disk_name": opt_file.upper(),
                    "cycles": total_cycles,
                    "calls": calls
                })
                print(f"✓ {name:15s} → {opt_level} ({calls:3d} calls, {total_cycles:10d} cycles)")
            else:
                print(f"⚠ {name:15s} → {opt_level} (not compiled yet)")

    print()
    print(f"📦 Found {len(to_sync)} modules to sync")
    print()

    if args.dry_run:
        print("🔍 Dry run - would sync:")
        for m in to_sync:
            print(f"   {m['file']} → {args.disk}::{m['disk_name']}")
        return 0

    # Sync to disk using mcopy
    synced = 0
    failed = 0

    for mod in to_sync:
        cmd = ["mcopy", "-i", args.disk, "-o", str(mod['file']), f"::{mod['disk_name']}"]
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            print(f"✓ Synced {mod['disk_name']}")
            synced += 1
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to sync {mod['disk_name']}: {e.stderr.decode()}")
            failed += 1

    print()
    print(f"✅ Synced {synced}/{len(to_sync)} modules to disk")
    if failed > 0:
        print(f"❌ Failed: {failed}")
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())
