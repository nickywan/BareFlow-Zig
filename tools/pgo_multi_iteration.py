#!/usr/bin/env python3
"""
Multi-Iteration PGO Workflow Automation

Automates the complete PGO cycle:
1. Baseline profiling (capture initial performance)
2. Classification (determine optimization levels)
3. Recompilation (apply optimizations)
4. Performance measurement (validate improvements)
5. Repeat iterations until convergence

Usage:
    python3 tools/pgo_multi_iteration.py [--max-iterations N] [--convergence-threshold PCT]
"""

import argparse
import json
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional


class PGOIterator:
    """Manages multi-iteration PGO workflow"""

    def __init__(self, max_iterations: int = 3, convergence_threshold: float = 0.02):
        self.max_iterations = max_iterations
        self.convergence_threshold = convergence_threshold
        self.results = []
        self.project_root = Path(__file__).parent.parent

    def run(self) -> bool:
        """Execute multi-iteration PGO workflow"""
        print("=" * 80)
        print("BareFlow Multi-Iteration PGO Workflow")
        print("=" * 80)
        print(f"Max iterations: {self.max_iterations}")
        print(f"Convergence threshold: {self.convergence_threshold * 100}%")
        print()

        # Iteration 0: Baseline
        print("[ITERATION 0] Baseline profiling (no optimizations)")
        baseline = self._run_iteration(0, is_baseline=True)
        if not baseline:
            print("❌ Baseline profiling failed")
            return False

        self.results.append(baseline)
        print(f"✅ Baseline complete: {baseline['total_cycles']:,} cycles")
        print()

        # Iterations 1-N: Optimize and measure
        for i in range(1, self.max_iterations + 1):
            print(f"[ITERATION {i}] PGO optimization cycle")

            result = self._run_iteration(i, is_baseline=False)
            if not result:
                print(f"❌ Iteration {i} failed")
                break

            self.results.append(result)

            # Calculate improvement over previous iteration
            prev_cycles = self.results[-2]['total_cycles']
            curr_cycles = result['total_cycles']
            improvement = (prev_cycles - curr_cycles) / prev_cycles

            print(f"✅ Iteration {i} complete: {curr_cycles:,} cycles")
            print(f"   Improvement: {improvement * 100:.2f}% over iteration {i-1}")

            # Check convergence
            if improvement < self.convergence_threshold:
                print(f"🎯 Converged! Improvement < {self.convergence_threshold * 100}%")
                break

            print()

        # Generate final report
        self._generate_report()
        return True

    def _run_iteration(self, iteration: int, is_baseline: bool) -> Optional[Dict]:
        """Run a single PGO iteration"""

        # Step 1: Build kernel
        print(f"  [1/4] Building kernel...")
        if not self._build_kernel():
            return None

        # Step 2: Capture profile
        print(f"  [2/4] Capturing profile...")
        profile_file = f"/tmp/pgo_iteration_{iteration}.json"
        if not self._capture_profile(profile_file):
            return None

        # Load profile
        with open(profile_file) as f:
            profile = json.load(f)

        if is_baseline:
            # Baseline: don't recompile
            return {
                'iteration': iteration,
                'is_baseline': True,
                'total_cycles': self._calculate_total_cycles(profile),
                'modules': profile['modules']
            }

        # Step 3: Classify and recompile
        print(f"  [3/4] Classifying and recompiling modules...")
        if not self._recompile_modules(profile_file):
            return None

        # Step 4: Rebuild kernel with optimized cache
        print(f"  [4/4] Rebuilding kernel with optimized modules...")
        if not self._rebuild_with_cache():
            return None

        # Measure optimized performance
        print(f"  [5/4] Measuring optimized performance...")
        optimized_profile_file = f"/tmp/pgo_iteration_{iteration}_optimized.json"
        if not self._capture_profile(optimized_profile_file):
            return None

        with open(optimized_profile_file) as f:
            optimized_profile = json.load(f)

        return {
            'iteration': iteration,
            'is_baseline': False,
            'total_cycles': self._calculate_total_cycles(optimized_profile),
            'modules': optimized_profile['modules']
        }

    def _build_kernel(self) -> bool:
        """Build the kernel"""
        try:
            result = subprocess.run(
                ['make', 'clean'],
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=30
            )

            result = subprocess.run(
                ['make', 'all'],
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=120
            )

            return result.returncode == 0
        except Exception as e:
            print(f"    ❌ Build failed: {e}")
            return False

    def _capture_profile(self, output_file: str) -> bool:
        """Capture profiling data from QEMU run"""
        try:
            # Run QEMU with serial output
            serial_file = "/tmp/pgo_serial.txt"
            result = subprocess.run(
                [
                    'timeout', '--foreground', '10',
                    'qemu-system-i386',
                    '-drive', 'file=fluid.img,format=raw',
                    '-serial', f'file:{serial_file}',
                    '-nographic'
                ],
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=15
            )

            # Extract JSON from serial output
            with open(serial_file) as f:
                content = f.read()

            # Find JSON between markers
            start = content.find('--- BEGIN JSON ---')
            end = content.find('--- END JSON ---')

            if start == -1 or end == -1:
                print("    ❌ No JSON markers found in output")
                return False

            json_str = content[start + len('--- BEGIN JSON ---'):end].strip()

            # Validate and save JSON
            profile = json.loads(json_str)
            with open(output_file, 'w') as f:
                json.dump(profile, f, indent=2)

            return True

        except Exception as e:
            print(f"    ❌ Profile capture failed: {e}")
            return False

    def _recompile_modules(self, profile_file: str) -> bool:
        """Recompile modules with PGO"""
        try:
            result = subprocess.run(
                [
                    'python3', 'tools/pgo_recompile.py',
                    profile_file,
                    '--apply',
                    '--output-dir', 'cache/i686'
                ],
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=60
            )

            if result.returncode != 0:
                print(f"    ❌ pgo_recompile.py failed")
                print(f"    stdout: {result.stdout}")
                print(f"    stderr: {result.stderr}")

            return result.returncode == 0

        except Exception as e:
            print(f"    ❌ Recompilation failed: {e}")
            return False

    def _rebuild_with_cache(self) -> bool:
        """Rebuild kernel with cached modules"""
        try:
            # List all .mod files in cache directory
            cache_dir = self.project_root / "cache" / "i686" / "default" / "default"
            mod_files = list(cache_dir.glob("*.mod"))

            # Extract module names (without _O1/_O2/_O3 suffix and .mod extension)
            module_names = set()
            for mod_file in mod_files:
                name = mod_file.stem  # Remove .mod
                # Remove optimization suffix if present
                for suffix in ['_O3', '_O2', '_O1', '_O0']:
                    if name.endswith(suffix):
                        name = name[:-len(suffix)]
                        break
                module_names.add(name)

            # Generate cache registry
            result = subprocess.run(
                [
                    'python3', 'tools/gen_cache_registry.py',
                    '--output', 'kernel/cache_registry.c',
                    '--modules'] + list(module_names),
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=30
            )

            if result.returncode != 0:
                print(f"    ❌ gen_cache_registry.py failed")
                print(f"    stdout: {result.stdout}")
                print(f"    stderr: {result.stderr}")
                return False

            # Rebuild kernel
            result = subprocess.run(
                ['make', 'all'],
                cwd=self.project_root,
                capture_output=True,
                text=True,
                timeout=120
            )

            return result.returncode == 0

        except Exception as e:
            print(f"    ❌ Cache rebuild failed: {e}")
            return False

    def _calculate_total_cycles(self, profile: Dict) -> int:
        """Calculate total cycles across all modules"""
        total = 0
        for module in profile.get('modules', []):
            total += module.get('total_cycles', 0)
        return total

    def _generate_report(self):
        """Generate performance report"""
        print()
        print("=" * 80)
        print("MULTI-ITERATION PGO RESULTS")
        print("=" * 80)
        print()

        # Summary table
        print("Iteration Summary:")
        print("-" * 80)
        print(f"{'Iter':<6} {'Type':<12} {'Total Cycles':>15} {'Improvement':>12} {'Speedup':>10}")
        print("-" * 80)

        baseline_cycles = self.results[0]['total_cycles']

        for result in self.results:
            iter_num = result['iteration']
            iter_type = "Baseline" if result['is_baseline'] else "Optimized"
            cycles = result['total_cycles']

            if iter_num == 0:
                improvement = "-"
                speedup = "1.00x"
            else:
                prev_cycles = self.results[iter_num - 1]['total_cycles']
                impr = (prev_cycles - cycles) / prev_cycles * 100
                improvement = f"{impr:+.2f}%"

                sp = baseline_cycles / cycles
                speedup = f"{sp:.2f}x"

            print(f"{iter_num:<6} {iter_type:<12} {cycles:>15,} {improvement:>12} {speedup:>10}")

        print("-" * 80)

        # Final statistics
        final_cycles = self.results[-1]['total_cycles']
        total_improvement = (baseline_cycles - final_cycles) / baseline_cycles * 100
        final_speedup = baseline_cycles / final_cycles

        print()
        print("Final Results:")
        print(f"  Baseline cycles:      {baseline_cycles:>15,}")
        print(f"  Final cycles:         {final_cycles:>15,}")
        print(f"  Total improvement:    {total_improvement:>14.2f}%")
        print(f"  Final speedup:        {final_speedup:>15.2f}x")
        print(f"  Iterations run:       {len(self.results):>15}")
        print()

        # Save report to file
        report_file = self.project_root / "build" / "pgo_report.json"
        with open(report_file, 'w') as f:
            json.dump({
                'iterations': self.results,
                'summary': {
                    'baseline_cycles': baseline_cycles,
                    'final_cycles': final_cycles,
                    'total_improvement_percent': total_improvement,
                    'final_speedup': final_speedup,
                    'iterations_run': len(self.results)
                }
            }, f, indent=2)

        print(f"📊 Full report saved to: {report_file}")
        print()


def main():
    parser = argparse.ArgumentParser(
        description='Multi-iteration PGO workflow automation'
    )
    parser.add_argument(
        '--max-iterations',
        type=int,
        default=3,
        help='Maximum number of optimization iterations (default: 3)'
    )
    parser.add_argument(
        '--convergence-threshold',
        type=float,
        default=0.02,
        help='Stop when improvement < threshold (default: 0.02 = 2%%)'
    )

    args = parser.parse_args()

    iterator = PGOIterator(
        max_iterations=args.max_iterations,
        convergence_threshold=args.convergence_threshold
    )

    success = iterator.run()
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
