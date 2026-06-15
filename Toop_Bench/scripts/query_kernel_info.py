#!/usr/bin/env python3
"""
query_kernel_info.py -- extract per-kernel launch statistics from ncu.

Scans all params/*.json files, runs ncu once per kernel, and extracts:
  - Registers per thread
  - Static shared memory per block
  - Dynamic shared memory per block
  - Threads per block (launch config)
  - Grid size
  - Theoretical occupancy
  - Achieved occupancy

Output: one CSV with one row per kernel, written to results/ncu/
        filename: kernel_info_<timestamp>.csv

Usage (from any directory):
  python Toop_Bench/scripts/query_kernel_info.py
  python Toop_Bench/scripts/query_kernel_info.py --params-dir Toop_Bench/params
  python Toop_Bench/scripts/query_kernel_info.py --output-dir Toop_Bench/results/ncu
  python Toop_Bench/scripts/query_kernel_info.py --dry-run
"""

import argparse
import csv
import json
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
SCRIPT_NAME = "query_kernel_info.py"
SCRIPT_DESC = (
    "Extract per-kernel launch statistics from ncu for use with the "
    "occupancy calculator. Scans all params/*.json files automatically."
)

# ncu metrics needed for occupancy calculator input
QUERY_METRICS = [
    "launch__registers_per_thread",
    "launch__shared_mem_per_block_static",
    "launch__shared_mem_per_block_dynamic",
    "launch__block_size",
    "launch__grid_size",
    "sm__maximum_warps_per_active_cycle_pct",
    "sm__warps_active.avg.pct_of_peak_sustained_active",
]

# ---------------------------------------------------------------------------
def find_params_files(params_dir: Path) -> list[Path]:
    files = sorted(params_dir.glob("*.json"))
    if not files:
        print(f"[ERROR] No JSON files found in: {params_dir}")
        sys.exit(1)
    return files


# ---------------------------------------------------------------------------
def load_params(params_file: Path) -> dict:
    with open(params_file, "r") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
def resolve_exe(params: dict, params_file: Path) -> Path:
    """Resolve executable path relative to the params JSON file."""
    raw = params.get("executable", "")
    if not raw:
        print(f"[ERROR] 'executable' not set in {params_file.name}")
        sys.exit(1)
    exe = (params_file.parent / raw).resolve()
    if not exe.exists():
        print(f"[ERROR] Executable not found: {exe}")
        print(f"        Check 'executable' in {params_file}")
        sys.exit(1)
    return exe


# ---------------------------------------------------------------------------
def resolve_config(params: dict, params_file: Path) -> Path:
    """Resolve config.json path relative to the params JSON file."""
    raw = params.get("config_file", "")
    if not raw:
        print(f"[ERROR] 'config_file' not set in {params_file.name}")
        sys.exit(1)
    cfg = (params_file.parent / raw).resolve()
    if not cfg.exists():
        print(f"[ERROR] config.json not found: {cfg}")
        print(f"        Check 'config_file' in {params_file}")
        sys.exit(1)
    return cfg


# ---------------------------------------------------------------------------
def build_ncu_cmd(exe: Path, config: Path, kernel_name: str) -> list[str]:
    metrics_arg = ",".join(QUERY_METRICS)
    return [
        "ncu",
        "--kernel-name", kernel_name,
        "--launch-count", "1",
        "--metrics", metrics_arg,
        "--csv",
        str(exe),
        "--headless",
        "--config", str(config.parent),
    ]


# ---------------------------------------------------------------------------
def run_ncu(cmd: list[str], kernel_name: str) -> str:
    print(f"  [ncu] {' '.join(cmd)}")
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=120,
        )
    except FileNotFoundError:
        print("[ERROR] 'ncu' not found. Is CUDA bin directory on PATH?")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print(f"[ERROR] ncu timed out for kernel: {kernel_name}")
        sys.exit(1)

    if result.returncode != 0:
        print(f"[ERROR] ncu failed for kernel: {kernel_name}")
        print(result.stderr)
        sys.exit(1)

    return result.stdout


# ---------------------------------------------------------------------------
def parse_ncu_csv(raw_output: str, kernel_name: str) -> dict:
    csv_lines = [
        line for line in raw_output.splitlines()
        if not line.startswith("==")
        and not line.startswith("[")
        and line.strip()
    ]

    if not csv_lines:
        print(f"[WARN] No CSV output for kernel: {kernel_name}")
        return {}

    import csv as csv_mod
    reader = csv_mod.DictReader(csv_lines)
    metrics = {}

    for row in reader:
        metric_name  = row.get("Metric Name", "").strip().strip('"')
        metric_value = row.get("Metric Value", "").strip().strip('"')
        if metric_name:
            metric_value = metric_value.replace(",", "")
            metrics[metric_name] = metric_value

    return metrics


# ---------------------------------------------------------------------------
def extract_info(metrics: dict, kernel_name: str) -> dict:
    def get(key: str) -> str:
        return metrics.get(key, "N/A")

    return {
        "kernel":                   kernel_name,
        "registers_per_thread":     get("launch__registers_per_thread"),
        "shared_mem_static_bytes":  get("launch__shared_mem_per_block_static"),
        "shared_mem_dynamic_bytes": get("launch__shared_mem_per_block_dynamic"),
        "threads_per_block":        get("launch__block_size"),
        "grid_size":                get("launch__grid_size"),
        "theoretical_occupancy_pct":get("sm__maximum_warps_per_active_cycle_pct"),
        "achieved_occupancy_pct":   get("sm__warps_active.avg.pct_of_peak_sustained_active"),
    }


# ---------------------------------------------------------------------------
def save_csv(rows: list[dict], output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = output_dir / f"kernel_info_{timestamp}.csv"

    fieldnames = [
        "kernel",
        "registers_per_thread",
        "shared_mem_static_bytes",
        "shared_mem_dynamic_bytes",
        "threads_per_block",
        "grid_size",
        "theoretical_occupancy_pct",
        "achieved_occupancy_pct",
    ]

    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    return csv_path


# ---------------------------------------------------------------------------
def main():
    script_dir = Path(__file__).parent

    parser = argparse.ArgumentParser(
        prog=SCRIPT_NAME,
        description=SCRIPT_DESC,
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Params files are scanned automatically from --params-dir.\n"
            "Each params/*.json must contain: 'kernel', 'executable', 'config_file'.\n\n"
            "Examples:\n"
            f"  python {SCRIPT_NAME}\n"
            f"  python {SCRIPT_NAME} --params-dir Toop_Bench/params\n"
            f"  python {SCRIPT_NAME} --dry-run\n"
        )
    )
    parser.add_argument(
        "--params-dir",
        type=Path,
        default=None,
        help="Directory containing params *.json files "
             "(default: params/ next to this script)"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Directory to write kernel_info_<timestamp>.csv "
             "(default: results/ncu/ next to this script)"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print ncu commands without executing them"
    )

    args = parser.parse_args()

    params_dir = (args.params_dir or script_dir.parent / "params").resolve()
    output_dir = (args.output_dir or script_dir.parent / "results" / "ncu").resolve()

    print(f"[INFO] Script:     {SCRIPT_NAME}")
    print(f"[INFO] Params dir: {params_dir}")
    print(f"[INFO] Output dir: {output_dir}")
    print()

    params_files = find_params_files(params_dir)
    print(f"[INFO] Found {len(params_files)} params file(s):")
    for f in params_files:
        print(f"         {f.name}")
    print()

    rows = []

    for params_file in params_files:
        params      = load_params(params_file)
        kernel_name = params.get("kernel", "")
        if not kernel_name:
            print(f"[WARN] 'kernel' not set in {params_file.name} -- skipping")
            continue

        exe    = resolve_exe(params, params_file)
        config = resolve_config(params, params_file)

        print(f"[INFO] Querying kernel: {kernel_name}")
        print(f"         exe:    {exe}")
        print(f"         config: {config}")

        cmd = build_ncu_cmd(exe, config, kernel_name)

        if args.dry_run:
            print(f"  [dry-run] {' '.join(cmd)}")
            rows.append({
                "kernel":                    kernel_name,
                "registers_per_thread":      "dry-run",
                "shared_mem_static_bytes":   "dry-run",
                "shared_mem_dynamic_bytes":  "dry-run",
                "threads_per_block":         "dry-run",
                "grid_size":                 "dry-run",
                "theoretical_occupancy_pct": "dry-run",
                "achieved_occupancy_pct":    "dry-run",
            })
            continue

        raw_output = run_ncu(cmd, kernel_name)
        metrics    = parse_ncu_csv(raw_output, kernel_name)
        info       = extract_info(metrics, kernel_name)
        rows.append(info)

        print(f"         registers_per_thread:      {info['registers_per_thread']}")
        print(f"         threads_per_block:          {info['threads_per_block']}")
        print(f"         theoretical_occupancy_pct:  {info['theoretical_occupancy_pct']}")
        print(f"         achieved_occupancy_pct:     {info['achieved_occupancy_pct']}")
        print()

    if not rows:
        print("[ERROR] No kernels processed.")
        sys.exit(1)

    csv_path = save_csv(rows, output_dir)
    print(f"[INFO] Results written to: {csv_path}")


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    main()