#!/usr/bin/env python3
"""
run_valgrind.py -- run Toop_Bench under valgrind for CPU-side memory checking.

Checks for memory leaks and invalid memory access on the host side.
Note: valgrind does not instrument CUDA kernels - CPU side only.

Usage (from any directory):
  python Toop_Bench/scripts/run_valgrind.py --exe x64/Release/Toop_Bench --config config.json
  python Toop_Bench/scripts/run_valgrind.py --exe x64/Release/Toop_Bench --config config.json --dry-run
"""

import argparse
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
SCRIPT_NAME = "run_valgrind.py"
SCRIPT_DESC = (
    "Run Toop_Bench under valgrind for CPU-side memory leak and invalid "
    "access detection. CUDA kernels are not instrumented."
)


# ---------------------------------------------------------------------------
def resolve_path(raw: str, label: str) -> Path:
    p = Path(raw).resolve()
    if not p.exists():
        print(f"[ERROR] {label} not found: {p}")
        sys.exit(1)
    return p


# ---------------------------------------------------------------------------
def build_valgrind_cmd(
    exe: Path,
    config_dir: Path,
    output_file: Path,
) -> list[str]:
    return [
        "valgrind",
        "--tool=memcheck",
        "--leak-check=full",
        "--show-leak-kinds=all",
        "--track-origins=yes",
        "--error-exitcode=1",
        f"--log-file={output_file}",
        str(exe),
        "--headless",
        "--config", str(config_dir),
    ]


# ---------------------------------------------------------------------------
def main():
    script_dir = Path(__file__).parent

    parser = argparse.ArgumentParser(
        prog=SCRIPT_NAME,
        description=SCRIPT_DESC,
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Required parameters:\n"
            "  --exe     path to Toop_Bench binary\n"
            "  --config  path to config.json\n\n"
            "Examples:\n"
            f"  python {SCRIPT_NAME} --exe x64/Release/Toop_Bench --config config.json\n"
            f"  python {SCRIPT_NAME} --exe x64/Release/Toop_Bench --config config.json --dry-run\n"
        )
    )
    parser.add_argument(
        "--exe",
        type=str,
        required=True,
        help="Path to Toop_Bench binary  [CRITICAL]"
    )
    parser.add_argument(
        "--config",
        type=str,
        required=True,
        help="Path to config.json  [CRITICAL]"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=None,
        help="Directory to write valgrind log "
             "(default: results/valgrind/ next to this script)"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print valgrind command without executing"
    )

    args = parser.parse_args()

    exe        = resolve_path(args.exe, "executable")
    config     = resolve_path(args.config, "config.json")
    config_dir = config.parent

    output_dir = (
        args.output_dir or script_dir.parent / "results" / "valgrind"
    ).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    timestamp   = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = output_dir / f"valgrind_{timestamp}.log"

    cmd = build_valgrind_cmd(exe, config_dir, output_file)

    print(f"[INFO] Script:     {SCRIPT_NAME}")
    print(f"[INFO] Executable: {exe}")
    print(f"[INFO] Config dir: {config_dir}")
    print(f"[INFO] Output:     {output_file}")
    print()

    if args.dry_run:
        print(f"[dry-run] {' '.join(cmd)}")
        return

    print(f"[INFO] Running valgrind (this will be slow - CPU emulation)...")
    print(f"[INFO] Log will be written to: {output_file}")
    print()

    try:
        result = subprocess.run(cmd, timeout=600)
    except FileNotFoundError:
        print("[ERROR] 'valgrind' not found. Is it installed?")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print("[ERROR] valgrind timed out after 600s.")
        sys.exit(1)

    print()
    if result.returncode == 0:
        print("[INFO] valgrind: no errors detected.")
    else:
        print(f"[WARN] valgrind: errors detected (exit code {result.returncode}).")
        print(f"[INFO] See full log: {output_file}")
        sys.exit(result.returncode)

    print(f"[INFO] Log written to: {output_file}")


# ---------------------------------------------------------------------------
if __name__ == "__main__":
    main()