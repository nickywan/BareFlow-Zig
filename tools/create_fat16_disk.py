#!/usr/bin/env python3
"""
Create a FAT16 disk image with test modules
This creates a 16MB FAT16 filesystem for testing disk I/O in BareFlow
Uses mtools (no sudo required)
"""

import os
import sys
import subprocess
import tempfile
import shutil
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.parent
BUILD_DIR = PROJECT_ROOT / "build"
CACHE_DIR = PROJECT_ROOT / "cache" / "i686"

def create_fat16_disk(output_path: Path, size_mb: int = 16):
    """Create a FAT16-formatted disk image using mtools"""

    print(f"[1] Creating {size_mb}MB FAT16 disk image...")

    # Create empty disk image
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_image = Path(tmpdir) / "disk.img"
        tmp_files = Path(tmpdir) / "files"
        tmp_files.mkdir()

        # Create empty file
        subprocess.run([
            "dd",
            "if=/dev/zero",
            f"of={tmp_image}",
            "bs=1M",
            f"count={size_mb}",
            "status=none"
        ], check=True)

        # Format as FAT16 using mkfs.fat
        print(f"[2] Formatting as FAT16...")
        result = subprocess.run([
            "mkfs.fat",
            "-F", "16",       # FAT16
            "-n", "BAREFLOW",  # Volume label
            str(tmp_image)
        ], capture_output=True, text=True)

        if result.returncode != 0:
            print("ERROR: mkfs.fat failed")
            print(result.stderr)
            return False

        print("✓ FAT16 filesystem created")

        # Prepare test files
        print(f"[3] Preparing test files...")

        # Create a simple test text file
        test_txt = tmp_files / "TEST.TXT"
        with open(test_txt, "w") as f:
            f.write("Hello from FAT16 filesystem!\n")
            f.write("This file was created by BareFlow tools.\n")

        print(f"  ✓ Created TEST.TXT")

        # Create README
        readme = tmp_files / "README.TXT"
        with open(readme, "w") as f:
            f.write("BareFlow FAT16 Test Disk\n")
            f.write("========================\n\n")
            f.write("This disk contains:\n")
            f.write("- TEST.TXT: Simple test file\n")
            f.write("- *.MOD files: Compiled modules\n")
            f.write("\n")
            f.write("Use this disk to test FAT16 filesystem driver\n")
            f.write("in BareFlow kernel.\n")

        print(f"  ✓ Created README.TXT")

        # Copy cached modules if they exist
        modules_copied = 0
        if CACHE_DIR.exists():
            for mod_file in CACHE_DIR.glob("*.mod"):
                # Copy to temp directory with uppercase name
                dest = tmp_files / mod_file.name.upper()
                shutil.copy(mod_file, dest)
                modules_copied += 1

            if modules_copied > 0:
                print(f"  ✓ Prepared {modules_copied} modules")

        # Copy files to FAT16 image using mcopy
        print(f"[4] Copying files to disk image...")

        for file in tmp_files.iterdir():
            result = subprocess.run([
                "mcopy",
                "-i", str(tmp_image),
                str(file),
                "::"
            ], capture_output=True, text=True)

            if result.returncode != 0:
                print(f"  ERROR: Failed to copy {file.name}")
                print(result.stderr)
            else:
                print(f"  ✓ Copied {file.name}")

        # Copy to output location
        shutil.copy(tmp_image, output_path)
        print(f"[5] Disk image saved to: {output_path}")

        # Print summary
        size_bytes = output_path.stat().st_size
        print(f"\n✓ FAT16 disk created successfully!")
        print(f"  Size: {size_bytes / (1024*1024):.1f} MB")
        print(f"  Format: FAT16")
        print(f"  Label: BAREFLOW")
        print(f"  Files: {len(list(tmp_files.iterdir()))}")
        print(f"\nTo use with QEMU:")
        print(f"  qemu-system-i386 -drive format=raw,file=fluid.img -drive format=raw,file={output_path.name}")

    return True

def main():
    """Main entry point"""
    output_path = BUILD_DIR / "fat16_test.img"

    # Ensure build directory exists
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    # Check for mkfs.fat
    if shutil.which("mkfs.fat") is None:
        print("ERROR: mkfs.fat not found")
        print("Install with: sudo apt-get install dosfstools")
        return 1

    # Check for mcopy
    if shutil.which("mcopy") is None:
        print("ERROR: mcopy (mtools) not found")
        print("Install with: sudo apt-get install mtools")
        return 1

    # Create disk
    if create_fat16_disk(output_path):
        return 0
    else:
        return 1

if __name__ == "__main__":
    sys.exit(main())
