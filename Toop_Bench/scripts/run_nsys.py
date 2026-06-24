#!/usr/bin/env python3
"""
run_nsys.py -- profile Toop_Bench with nsys and extract per-kernel timing.

Runs nsys profile once, captures all CUDA kernels, queries the SQLite
output, and writes a timestamped CSV with per-kernel timing data.

Usage (from any directory):
  python Toop_Bench/scripts/run_nsys.py --exe x64/Release/Toop_Bench --config config.json
  python Toop_Bench/scripts/run_nsys.py --exe x64/Release/Toop_Bench --config config.json --dry-run
  python Toop_Bench/scripts/run_nsys.py --list-params
"""

import argparse
import csv
import sqlite3
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
SCRIPT_NAME = "run_nsys.py"
SCRIPT_DESC = (
    "Profile Toop_Bench with nsys and extract per-kernel GPU timing. "
    "Runs the exe once and captures all kernels in a single profile."
)

PARAMS_DESC = {
    "--exe":          "Path to Toop_Bench binary  [CRITICAL]",
    "--config":       "Path to config.json  [CRITICAL]",
    "--output-dir":   "Directory for CSV and nsys report output "
                      "(default: Toop_Bench/results/nsys/ next to this script)",
    "--extra-args":   "Extra arguments passed to nsys profile "
                      "(default: '--trace cuda,osrt')",
    "--dry-run":      "Print nsys command without executing",
}

SQLITE_QUERY = """
    SELECT s.value,
           sum(k.end - k.start) / 1000000.0  AS total_ms,
           count(*)                            AS calls,
           avg(k.end - k.start) / 1000.0      AS avg_us
    FROM CUPTI_ACTIVITY_KIND_KERNEL k
    JOIN StringIds s ON k.shortName = s.id
    GROUP BY k.shortName
    ORDER BY total_ms DESC
"""


# ---------------------------------------------------------------------------
def print_list_params():
    print(f"\n{SCRIPT_NAME} -- parameter reference\n")
    print("Parameters:")
    for param, desc in PARAMS_DESC.items():
        print(f"  {param:<16}  {desc}")
    print()
    print("Examples:")
    print(f"  python {SCRIPT_NAME} --exe x64/Release/Toop_Bench --config config.json")
    print(f"  python {SCRIPT_NAME} --exe x64/Release/Toop_Bench --config config.json --dry-run")


# ---------------------------------------------------------------------------
def resolve_path(raw: str, label: str) -> Path:
    p = Path(raw).resolve()
    if not p.exists():
        print(f"[ERROR] {label} not found: {p}")
        sys.exit(1)
    return p


# ---------------------------------------------------------------------------
def build_nsys_cmd(
    exe: Path,
    config_dir: Path,
    report_path: Path,
    extra_args: str,
) -> list[str]:
    cmd = ["nsys", "profile"]

    if extra_args:
        cmd += extra_args.split()

    cmd += [
        "--output", str(report_path),
        "--export", "sqlite",
        "--force-overwrite", "true",
        str(exe),
        "--headless",
        "--config", str(config_dir),
    ]

    return cmd


# ---------------------------------------------------------------------------
def run_nsys(cmd: list[str]) -> None:
    print(f"[INFO] Running: {' '.join(cmd)}")
    print()

    try:
        result = subprocess.run(cmd, timeout=600)
    except FileNotFoundError:
        print("[ERROR] 'nsys' not found. Is nsys on PATH?")
        print("        Inside Docker: export PATH=/opt/nvidia/nsight-compute/"
              "2024.2.0/host/target-linux-x64:$PATH")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print("[ERROR] nsys timed out after 600s.")
        sys.exit(1)

    if result.returncode != 0:
        print(f"[ERROR] nsys failed with exit code {result.returncode}")
        sys.exit(1)


# ---------------------------------------------------------------------------
def query_sqlite(sqlite_path: Path) -> list[dict]:
    if not sqlite_path.exists():
        print(f"[ERROR] SQLite file not found: {sqlite_path}")
        print("        nsys may have failed to generate it.")
        sys.exit(1)

    conn = sqlite3.connect(sqlite_path)
    cursor = conn.execute(SQLITE_QUERY)
    rows = []

    for row in cursor.fetchall():
        rows.append({
            "kernel":    row[0],
            "total_ms":  round(row[1], 6),
            "calls":     row[2],
            "avg_us":    round(row[3], 3),
        })

    conn.close()
    return rows


# ---------------------------------------------------------------------------
def save_csv(rows: list[dict], output_dir: Path, timestamp: str) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    csv_path = output_dir / f"nsys_timing_{timestamp}.csv"

    fieldnames = ["kernel", "total_ms", "calls", "avg_us"]
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    return csv_path


# ---------------------------------------------------------------------------
def print_table(rows: list[dict]) -> None:
    total = sum(r["total_ms"] for r in rows)
    print(f"  {'Kernel':<40} {'total_ms':>10} {'calls':>8} {'avg_us':>10} {'%':>6}")
    print(f"  {'-'*40} {'-'*10} {'-'*8} {'-'*10} {'-'*6}")
    for r in rows:
        pct = (r["total_ms"] / total * 100) if total > 0 else 0
        print(f"  {r['kernel']:<40} {r['total_ms']:>10.3f} {r['calls']:>8} "
              f"{r['avg_us']:>10.3f} {pct:>5.1f}%")
    print(f"  {'TOTAL':<40} {total:>10.3f}")


# ---------------------------------------------------------------------------
def main():
    script_dir = Path(__file__).parent

    parser = argparse.ArgumentParser(
        prog=SCRIPT_NAME,
        description=SCRIPT_DESC,
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Critical parameters:\n"
            "  --exe     path to Toop_Bench binary\n"
            "  --config  path to config.json\n\n"
            f"Run 'python {SCRIPT_NAME} --list-params' for full reference.\n\n"
            "Examples:\n"
            f"  python {SCRIPT_NAME} "
            f"--exe x64/Release/Toop_Bench --config config.json\n"
            f"  python {SCRIPT_NAME} "
            f"--exe x64/Release/Toop_Bench --config config.json --dry-run\n"
        )
    )
    parser.add_argument(
        "--exe",
        type=str,
        default=None,
        help="Path to Toop_Bench binary  [CRITICAL]"
    )
    parser.add_argument(
        "--config",
        type=str,
        default=None,
        help="Path to config.json  [CRITICAL]"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Directory for CSV and nsys report output"
    )
    parser.add_argument(
        "--extra-args",
        type=str,
        default="--trace cuda,osrt",
        help="Extra arguments passed to nsys profile"
    )
    parser.add_argument(
        "--list-params",
        action="store_true",
        help="Show all parameters with descriptions, then exit"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print nsys command without executing"
    )

    args = parser.parse_args()

    if args.list_params:
        print_list_params()
        return

    if not args.exe or not args.config:
        print(f"[ERROR] --exe and --config are required.")
        print(f"        Run 'python {SCRIPT_NAME} --list-params' for help.")
        sys.exit(1)

    exe        = resolve_path(args.exe, "executable")
    config     = resolve_path(args.config, "config.json")
    config_dir = config.parent

    output_dir = (
        args.output_dir or script_dir.parent / "results" / "nsys"
    ).resolve()

    timestamp   = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_path = output_dir / f"toop_profile_{timestamp}"
    sqlite_path = Path(str(report_path) + ".sqlite")

    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"[INFO] Script:      {SCRIPT_NAME}")
    print(f"[INFO] Executable:  {exe}")
    print(f"[INFO] Config dir:  {config_dir}")
    print(f"[INFO] Output dir:  {output_dir}")
    print(f"[INFO] Report:      {report_path}.nsys-rep")
    print(f"[INFO] SQLite:      {sqlite_path}")
    print()

    cmd = build_nsys_cmd(exe, config_dir, report_path, args.extra_args)

    if args.dry_run:
        print(f"[dry-run] {' '.join(cmd)}")
        return

    run_nsys(cmd)

    print()
    print("[INFO] Querying kernel timing from SQLite...")
    rows = query_sqlite(sqlite_path)

    if not rows:
        print("[WARN] No kernel data found in SQLite output.")
        return

    print()
    print_table(rows)

    csv_path = save_csv(rows, output_dir, timestamp)
    print()
    print(f"[INFO] CSV written to: {csv_path}")
    print(f"[INFO] nsys report:    {report_path}.nsys-rep")
    print(f"       Copy the .nsys-rep to Windows to open in Nsight Systems GUI:")
    print(f"       docker cp <container_id>:{report_path}.nsys-rep ./toop_profile.nsys-rep")


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    main()