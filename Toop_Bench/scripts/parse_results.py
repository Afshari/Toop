#!/usr/bin/env python3
"""
parse_results.py -- aggregate ncu sweep CSVs into a summary table.

Scans a results directory for sweep CSV files, merges them into one
summary CSV sorted by kernel and threads_per_block.

Usage (from any directory):
  python Toop_Bench/scripts/parse_results.py
  python Toop_Bench/scripts/parse_results.py --input Toop_Bench/results/ncu
  python Toop_Bench/scripts/parse_results.py --input Toop_Bench/results/ncu --output summary.csv
"""

import argparse
import csv
import sys
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
SCRIPT_NAME = "parse_results.py"
SCRIPT_DESC = (
    "Aggregate ncu sweep CSV files into one summary table. "
    "Scans --input directory for *_sweep_*.csv files."
)


# ---------------------------------------------------------------------------
def find_sweep_csvs(input_dir: Path) -> list[Path]:
    files = sorted(input_dir.glob("*_sweep_*.csv"))
    if not files:
        print(f"[ERROR] No sweep CSV files found in: {input_dir}")
        print(f"        Expected files matching: *_sweep_*.csv")
        sys.exit(1)
    return files


# ---------------------------------------------------------------------------
def load_csv(csv_path: Path) -> list[dict]:
    with open(csv_path, "r", newline="") as f:
        reader = csv.DictReader(f)
        return list(reader)


# ---------------------------------------------------------------------------
def merge_rows(all_files: list[Path]) -> list[dict]:
    merged = []
    for csv_path in all_files:
        rows = load_csv(csv_path)
        for row in rows:
            row["source_file"] = csv_path.name
        merged.extend(rows)
        print(f"  [loaded] {csv_path.name} ({len(rows)} rows)")
    return merged


# ---------------------------------------------------------------------------
def sort_rows(rows: list[dict]) -> list[dict]:
    def sort_key(row):
        kernel = row.get("kernel", "")
        try:
            tpb = int(row.get("threads_per_block", 0))
        except ValueError:
            tpb = 0
        return (kernel, tpb)
    return sorted(rows, key=sort_key)


# ---------------------------------------------------------------------------
def save_summary(rows: list[dict], output_path: Path) -> None:
    if not rows:
        print("[ERROR] No rows to write.")
        sys.exit(1)

    output_path.parent.mkdir(parents=True, exist_ok=True)

    # put kernel and threads_per_block first, source_file last
    all_keys = list(rows[0].keys())
    priority = ["kernel", "threads_per_block"]
    tail     = ["source_file"]
    middle   = [k for k in all_keys if k not in priority and k not in tail]
    fieldnames = priority + middle + tail

    with open(output_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
def print_summary(rows: list[dict]) -> None:
    kernels = sorted(set(r.get("kernel", "") for r in rows))
    print(f"\n[INFO] Kernels found: {len(kernels)}")
    for k in kernels:
        k_rows = [r for r in rows if r.get("kernel") == k]
        tpb_vals = [r.get("threads_per_block", "?") for r in k_rows]
        print(f"         {k}: tpb={tpb_vals}")


# ---------------------------------------------------------------------------
def main():
    script_dir = Path(__file__).parent

    parser = argparse.ArgumentParser(
        prog=SCRIPT_NAME,
        description=SCRIPT_DESC,
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            f"  python {SCRIPT_NAME}\n"
            f"  python {SCRIPT_NAME} --input Toop_Bench/results/ncu\n"
            f"  python {SCRIPT_NAME} --input Toop_Bench/results/ncu --output my_summary.csv\n"
        )
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=None,
        help="Directory containing *_sweep_*.csv files "
             "(default: results/ncu/ next to this script)"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output summary CSV path "
             "(default: results/summary_<timestamp>.csv next to this script)"
    )

    args = parser.parse_args()

    input_dir = (args.input or script_dir.parent / "results" / "ncu").resolve()

    timestamp    = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_path  = (
        args.output or script_dir.parent / "results" / f"summary_{timestamp}.csv"
    ).resolve()

    print(f"[INFO] Script:     {SCRIPT_NAME}")
    print(f"[INFO] Input dir:  {input_dir}")
    print(f"[INFO] Output:     {output_path}")
    print()

    if not input_dir.exists():
        print(f"[ERROR] Input directory not found: {input_dir}")
        sys.exit(1)

    csv_files = find_sweep_csvs(input_dir)
    print(f"[INFO] Found {len(csv_files)} sweep file(s):")
    for f in csv_files:
        print(f"         {f.name}")
    print()

    rows = merge_rows(csv_files)
    rows = sort_rows(rows)

    print_summary(rows)

    save_summary(rows, output_path)
    print(f"\n[INFO] Summary written to: {output_path}")
    print(f"[INFO] Total rows: {len(rows)}")


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    main()