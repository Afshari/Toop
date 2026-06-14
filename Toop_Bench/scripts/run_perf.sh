#!/bin/bash
# run_perf.sh - profile Toop_Bench with perf for CPU-side host overhead.
#
# Captures: kernel launch overhead, cudaMemset, synchronization,
# and any CPU-side hotspots in HairSimulator::Step() orchestration.
#
# Note: requires perf_event_paranoid=-1 on the host (setup_aws.sh sets this).
#       Run inside the bench Docker service with --cap-add SYS_ADMIN.
#
# Usage (from any directory):
#   bash Toop_Bench/scripts/run_perf.sh --exe x64/Release/Toop_Bench --config config.json
#   bash Toop_Bench/scripts/run_perf.sh --exe x64/Release/Toop_Bench --config config.json --dry-run

set -e

# ---------------------------------------------------------------------------
# Defaults
# ---------------------------------------------------------------------------
EXE=""
CONFIG=""
OUTPUT_DIR=""
DRY_RUN=0
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_OUTPUT_DIR="$(dirname "$SCRIPT_DIR")/results/perf"

# ---------------------------------------------------------------------------
# Usage
# ---------------------------------------------------------------------------
usage() {
    echo ""
    echo "Usage: bash run_perf.sh --exe <path> --config <path> [options]"
    echo ""
    echo "Required:"
    echo "  --exe <path>         Path to Toop_Bench binary"
    echo "  --config <path>      Path to config.json"
    echo ""
    echo "Optional:"
    echo "  --output-dir <path>  Directory for perf output (default: results/perf/)"
    echo "  --dry-run            Print commands without executing"
    echo "  --help               Show this message"
    echo ""
    echo "Examples:"
    echo "  bash Toop_Bench/scripts/run_perf.sh --exe x64/Release/Toop_Bench --config config.json"
    echo "  bash Toop_Bench/scripts/run_perf.sh --exe x64/Release/Toop_Bench --config config.json --dry-run"
    echo ""
}

# ---------------------------------------------------------------------------
# Parse args
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --exe)         EXE="$2";        shift 2 ;;
        --config)      CONFIG="$2";     shift 2 ;;
        --output-dir)  OUTPUT_DIR="$2"; shift 2 ;;
        --dry-run)     DRY_RUN=1;       shift   ;;
        --help)        usage; exit 0             ;;
        *)
            echo "[ERROR] Unknown argument: $1"
            usage
            exit 1
            ;;
    esac
done

# ---------------------------------------------------------------------------
# Validate required args
# ---------------------------------------------------------------------------
if [[ -z "$EXE" || -z "$CONFIG" ]]; then
    echo "[ERROR] --exe and --config are required."
    usage
    exit 1
fi

EXE=$(realpath "$EXE")
CONFIG=$(realpath "$CONFIG")
CONFIG_DIR=$(dirname "$CONFIG")

if [[ ! -f "$EXE" ]]; then
    echo "[ERROR] Executable not found: $EXE"
    exit 1
fi

if [[ ! -f "$CONFIG" ]]; then
    echo "[ERROR] config.json not found: $CONFIG"
    exit 1
fi

OUTPUT_DIR="${OUTPUT_DIR:-$DEFAULT_OUTPUT_DIR}"
mkdir -p "$OUTPUT_DIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
PERF_DATA="$OUTPUT_DIR/perf_${TIMESTAMP}.data"
PERF_REPORT="$OUTPUT_DIR/perf_${TIMESTAMP}_report.txt"

# ---------------------------------------------------------------------------
# Print info
# ---------------------------------------------------------------------------
echo "[INFO] Script:      run_perf.sh"
echo "[INFO] Executable:  $EXE"
echo "[INFO] Config dir:  $CONFIG_DIR"
echo "[INFO] Output dir:  $OUTPUT_DIR"
echo "[INFO] perf data:   $PERF_DATA"
echo "[INFO] perf report: $PERF_REPORT"
echo ""

# ---------------------------------------------------------------------------
# Check perf is available
# ---------------------------------------------------------------------------
if ! command -v perf &> /dev/null; then
    echo "[ERROR] 'perf' not found. Is linux-tools-generic installed?"
    exit 1
fi

# Check perf_event_paranoid
PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "unknown")
if [[ "$PARANOID" != "-1" ]]; then
    echo "[WARN] perf_event_paranoid=$PARANOID (expected -1)"
    echo "[WARN] Run: sudo sysctl -w kernel.perf_event_paranoid=-1"
    echo "[WARN] Or add to /etc/sysctl.d/99-perf.conf and reboot."
    echo ""
fi

# ---------------------------------------------------------------------------
# Build commands
# ---------------------------------------------------------------------------
RECORD_CMD=(
    perf record
    -g
    -F 99
    -o "$PERF_DATA"
    "$EXE"
    --headless
    --config "$CONFIG_DIR"
)

REPORT_CMD=(
    perf report
    -i "$PERF_DATA"
    --stdio
    --no-children
)

# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
if [[ "$DRY_RUN" -eq 1 ]]; then
    echo "[dry-run] ${RECORD_CMD[*]}"
    echo "[dry-run] ${REPORT_CMD[*]} > $PERF_REPORT"
    exit 0
fi

echo "[INFO] Running perf record..."
"${RECORD_CMD[@]}"

echo ""
echo "[INFO] Generating perf report..."
"${REPORT_CMD[@]}" > "$PERF_REPORT"

echo "[INFO] Report written to: $PERF_REPORT"
echo ""
echo "[INFO] Top functions (first 30 lines):"
head -50 "$PERF_REPORT" | grep -v "^#" | head -30