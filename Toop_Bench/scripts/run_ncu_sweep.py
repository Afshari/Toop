#!/usr/bin/env python3
"""
run_ncu_sweep.py -- sweep threads_per_block for a single kernel using ncu.

Reads a per-kernel params JSON, runs ncu for each threads_per_block value,
collects the metrics defined in the JSON, and writes one timestamped CSV.

Usage (from any directory):
  python Toop_Bench/scripts/run_ncu_sweep.py --params Toop_Bench/params/k_solve_constraints.json
  python Toop_Bench/scripts/run_ncu_sweep.py --params Toop_Bench/params/k_integrate.json --dry-run
  python Toop_Bench/scripts/run_ncu_sweep.py --list-params
"""

import argparse
import csv
import json
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
SCRIPT_NAME = "run_ncu_sweep.py"
SCRIPT_DESC = (
    "Sweep threads_per_block for a single CUDA kernel using ncu. "
    "Reads sweep config from a per-kernel params JSON file."
)

PARAMS_TEMPLATE = {
    "kernel":      "Exact CUDA kernel function name (used with --kernel-name)  [CRITICAL]",
    "cli_name":    "Kernel name as passed to the executable CLI  [CRITICAL]",
    "executable":  "Path to Toop_Bench binary, relative to this JSON file  [CRITICAL]",
    "config_file": "Path to config.json, relative to this JSON file",
    "results_dir": "Directory for CSV output, relative to this JSON file",
    "ncu": {
        "output_dir":  "Directory for raw ncu reports, relative to this JSON file",
        "extra_args":  "Extra arguments passed verbatim to ncu (e.g. '--target-processes all')",
        "metrics":     "List of ncu metric names to collect",
    },
    "sweep": {
        "threads_per_block": "List of block sizes to sweep, e.g. [64, 128, 256, 512]  [CRITICAL]",
    },
}

PARAMS_EXAMPLE = {
    "kernel":      "k_solve_constraints_xpbd",
    "cli_name":    "k_solve_constraints_xpbd",
    "executable":  "../../x64/Release/Toop_Bench",
    "config_file": "../../config.json",
    "results_dir": "results",
    "ncu": {
        "output_dir":  "results/ncu_reports",
        "extra_args":  "--target-processes all",
        "metrics": [
            "sm__throughput.avg.pct_of_peak_sustained_elapsed",
            "dram__bytes_read.sum",
            "smsp__sass_average_data_bytes_per_sector_mem_global_op_ld.pct",
            "smsp__warp_issue_stalled_long_scoreboard_per_warp_active.pct",
            "sm__occupancy.avg.pct_of_peak_sustained_active",
        ],
    },
    "sweep": {
        "threads_per_block": [64, 128, 256, 512],
    },
}


# ---------------------------------------------------------------------------
def print_list_params():
    print(f"\n{SCRIPT_NAME} -- parameter reference")
    print(f"Pass params file with: python {SCRIPT_NAME} --params path/to/k_kernel.json\n")

    print("Parameters:")
    print(f"  kernel                   {PARAMS_TEMPLATE['kernel']}")
    print(f"  cli_name                 {PARAMS_TEMPLATE['cli_name']}")
    print(f"  executable               {PARAMS_TEMPLATE['executable']}")
    print(f"  config_file              {PARAMS_TEMPLATE['config_file']}")
    print(f"  results_dir              {PARAMS_TEMPLATE['results_dir']}")
    print(f"  ncu.output_dir           {PARAMS_TEMPLATE['ncu']['output_dir']}")
    print(f"  ncu.extra_args           {PARAMS_TEMPLATE['ncu']['extra_args']}")
    print(f"  ncu.metrics              {PARAMS_TEMPLATE['ncu']['metrics']}")
    print(f"  sweep.threads_per_block  {PARAMS_TEMPLATE['sweep']['threads_per_block']}")

    print("\nExample params JSON:")
    print(json.dumps(PARAMS_EXAMPLE, indent=4))


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
def resolve_dir(params: dict, key: str, params_file: Path) -> Path:
    """Resolve a directory path from params relative to the params JSON file."""
    raw = params.get(key, "results")
    return (params_file.parent / raw).resolve()


# ---------------------------------------------------------------------------
def build_ncu_cmd(
    exe: Path,
    config: Path,
    kernel_name: str,
    threads_per_block: int,
    metrics: list[str],
    extra_args: str,
    report_path: Path,
) -> list[str]:
    cmd = ["ncu"]

    if extra_args:
        cmd += extra_args.split()

    cmd += [
        "--kernel-name", kernel_name,
        "--launch-count", "1",
        "--metrics", ",".join(metrics),
        "--csv",
        "--export", str(report_path),
    ]

    cmd += [
        str(exe),
        "--headless",
        "--config", str(config.parent),
        "--threads-per-block", str(threads_per_block),
    ]

    return cmd


# ---------------------------------------------------------------------------
def run_ncu(cmd: list[str], kernel_name: str, tpb: int) -> str:
    print(f"    [ncu] threads_per_block={tpb}")
    print(f"          {' '.join(cmd)}")

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300,
        )
    except FileNotFoundError:
        print("[ERROR] 'ncu' not found. Is CUDA bin directory on PATH?")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print(f"[ERROR] ncu timed out for kernel={kernel_name} tpb={tpb}")
        sys.exit(1)

    if result.returncode != 0:
        print(f"[ERROR] ncu failed for kernel={kernel_name} tpb={tpb}")
        print(result.stderr)
        sys.exit(1)

    return result.stdout


# ---------------------------------------------------------------------------
def parse_ncu_csv(raw_output: str, kernel_name: str, tpb: int) -> dict:
    """
    Parse ncu --csv output into a flat dict of metric_name -> value.
    ncu emits comment lines starting with '==' before the CSV block.
    """
    csv_lines = [
        line for line in raw_output.splitlines()
        if not line.startswith("==") and line.strip()
    ]

    if not csv_lines:
        print(f"[WARN] No CSV output for kernel={kernel_name} tpb={tpb}")
        return {}

    import csv as csv_mod
    reader = csv_mod.DictReader(csv_lines)
    metrics = {}

    for row in reader:
        metric_name  = row.get("Metric Name", "").strip()
        metric_value = row.get("Metric Value", "").strip()
        if metric_name:
            metric_value = metric_value.replace(",", "")
            metrics[metric_name] = metric_value

    return metrics


# ---------------------------------------------------------------------------
def run_sweep(
    params: dict,
    params_file: Path,
    exe: Path,
    config: Path,
    ncu_report_dir: Path,
    dry_run: bool,
) -> list[dict]:
    kernel_name = params["kernel"]
    tpb_values  = params["sweep"]["threads_per_block"]
    metrics     = params["ncu"]["metrics"]
    extra_args  = params["ncu"].get("extra_args", "")

    print(f"[INFO] Kernel:             {kernel_name}")
    print(f"[INFO] threads_per_block:  {tpb_values}")
    print(f"[INFO] Metrics count:      {len(metrics)}")
    print()

    rows = []

    for tpb in tpb_values:
        timestamp   = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_path = ncu_report_dir / f"{kernel_name}_tpb{tpb}_{timestamp}"

        cmd = build_ncu_cmd(
            exe, config, kernel_name, tpb,
            metrics, extra_args, report_path,
        )

        if dry_run:
            print(f"  [dry-run] tpb={tpb}")
            print(f"            {' '.join(cmd)}")
            row = {"kernel": kernel_name, "threads_per_block": tpb}
            for m in metrics:
                row[m] = "dry-run"
            rows.append(row)
            continue

        raw_output   = run_ncu(cmd, kernel_name, tpb)
        metric_vals  = parse_ncu_csv(raw_output, kernel_name, tpb)

        row = {"kernel": kernel_name, "threads_per_block": tpb}
        for m in metrics:
            row[m] = metric_vals.get(m, "N/A")
        rows.append(row)

        # print a short summary line per tpb
        occupancy = metric_vals.get(
            "sm__occupancy.avg.pct_of_peak_sustained_active", "N/A")
        coalescing = metric_vals.get(
            "smsp__sass_average_data_bytes_per_sector_mem_global_op_ld.pct", "N/A")
        print(f"    --> tpb={tpb:4d}  occupancy={occupancy}%  coalescing_ld={coalescing}%")
        print()

    return rows


# ---------------------------------------------------------------------------
def save_csv(rows: list[dict], results_dir: Path, kernel_name: str) -> Path:
    if not rows:
        return None

    results_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path  = results_dir / f"{kernel_name}_sweep_{timestamp}.csv"

    fieldnames = list(rows[0].keys())

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
            "Critical params to check in the JSON before running:\n"
            "  executable            -- path to Toop_Bench binary\n"
            "  sweep.threads_per_block -- block sizes to sweep\n\n"
            f"Run 'python {SCRIPT_NAME} --list-params' for full parameter reference.\n\n"
            "Examples:\n"
            f"  python {SCRIPT_NAME} --params Toop_Bench/params/k_solve_constraints.json\n"
            f"  python {SCRIPT_NAME} --params Toop_Bench/params/k_integrate.json --dry-run\n"
        )
    )
    parser.add_argument(
        "--params",
        type=Path,
        required=False,
        default=None,
        help="Path to per-kernel params JSON file  [CRITICAL]"
    )
    parser.add_argument(
        "--list-params",
        action="store_true",
        help="Show all parameters with descriptions and example JSON, then exit"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print ncu commands without executing them"
    )

    args = parser.parse_args()

    if args.list_params:
        print_list_params()
        return

    if not args.params:
        print(f"[ERROR] --params is required.")
        print(f"        Example: python {SCRIPT_NAME} "
              f"--params Toop_Bench/params/k_solve_constraints.json")
        print(f"        Run 'python {SCRIPT_NAME} --list-params' for help.")
        sys.exit(1)

    params_file = args.params.resolve()
    if not params_file.exists():
        print(f"[ERROR] Params file not found: {params_file}")
        sys.exit(1)

    params = load_params(params_file)

    exe         = resolve_exe(params, params_file)
    config      = resolve_config(params, params_file)
    results_dir = resolve_dir(params, "results_dir", params_file)
    ncu_dir     = (params_file.parent / params["ncu"]["output_dir"]).resolve()

    ncu_dir.mkdir(parents=True, exist_ok=True)

    print(f"[INFO] Script:       {SCRIPT_NAME}")
    print(f"[INFO] Params file:  {params_file}")
    print(f"[INFO] Executable:   {exe}")
    print(f"[INFO] Config:       {config}")
    print(f"[INFO] Results dir:  {results_dir}")
    print(f"[INFO] ncu reports:  {ncu_dir}")
    if args.dry_run:
        print(f"[INFO] Mode:         dry-run")
    print()

    rows = run_sweep(
        params, params_file,
        exe, config,
        ncu_dir,
        args.dry_run,
    )

    csv_path = save_csv(rows, results_dir, params["kernel"])
    if csv_path:
        print(f"[INFO] Results written to: {csv_path}")


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    main()