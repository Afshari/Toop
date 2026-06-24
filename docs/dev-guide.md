# Toop — Developer Guide

A real-time fur simulation (hair strands on a bouncing sphere) built with C++17 and CUDA 12.5.

## Contents

- [Project Structure](#project-structure)
- [Build](#build)
- [Workflow on AWS](#workflow-on-aws)
- [Profiling](#profiling)
- [Tests](#tests)
- [Git](#git)
- [Config](#config)

---

## Project Structure

```
Toop/
├── Toop/                      # Entry point and interactive exe (main.cpp, AppRunner)
├── Toop_CPU/                  # CPU-side lib (Config, CLParser, Camera, SpherePhysics)
├── Toop_GPU/                  # GPU lib (HairSimulator, HairSim.cu, Renderer)
├── Toop_Tests/                # GoogleTest unit tests
├── Toop_Bench/                # Profiling and benchmarking infrastructure
│   ├── scripts/               # Python and shell profiling scripts
│   ├── params/                # Per-kernel ncu sweep configs (JSON)
│   └── results/               # Generated output (gitignored)
├── cmake/                     # Shared CMake helpers (targets.cmake)
├── config.json                # Runtime simulation parameters
├── Dockerfile                 # Headless AWS build (Tesla T4)
├── docker-compose.yaml        # Service definitions
├── scripts/                   # Shell scripts
│   └── setup-aws.sh           # One-time AWS instance setup
└── .gitattributes             # Line ending rules for shell scripts
```

---

## Build

### Windows (Visual Studio 2022)

Open `Toop.sln` and build in Release x64.

Preprocessor defines (set in project properties):
- `TOOP_HEADLESS` -- strips OpenGL, GLFW, GLAD, ImGui
- `TOOP_DEBUG` -- enables DebugUI and DebugRenderer overlays

### Linux / Docker

Build the Docker image:
```bash
docker compose build
```

Rebuild from scratch after major changes:
```bash
docker compose build --no-cache
```

---

## Workflow on AWS

### First-time instance setup

Run once on a fresh EC2 instance:
```bash
chmod +x scripts/setup-aws.sh && ./scripts/setup-aws.sh
```

Log out and back in after it completes (required for Docker group membership).

### Standard workflow

**Everything runs inside the shell container.** This gives you access to
the binary, all scripts, and the results directory in one place.

Step 1 -- open a privileged shell (required for ncu and perf):
```bash
docker compose --profile shell run shell
```

Step 2 -- inside the container, run whatever you need:
```bash
# Quick sanity check - run the simulation
/app/x64/Release/Toop_Bench --headless --config /app

# Run profiling scripts (see Profiling section below)
python Toop_Bench/scripts/query_kernel_info.py
```

Step 3 -- results are written to `/app/Toop_Bench/results/` inside the
container. Copy them out before exiting:
```bash
# From the host, while the container is running
docker cp <container_id>:/app/Toop_Bench/results ./Toop_Bench/results
```

Check the container ID with:
```bash
docker ps
```

### Pull latest code and rebuild

```bash
git pull origin main
docker compose build
```

---

## Profiling

All scripts are run from inside the shell container.
Open the shell first (privileged mode is required for ncu):
```bash
docker compose --profile shell run shell
```

Then run any of the following from `/app` inside the container.
Results are written to `Toop_Bench/results/`. Copy them out before exiting:
```bash
docker cp <container_id>:/app/Toop_Bench/results ./Toop_Bench/results
```

---

### NCU -- kernel info

Extracts registers per thread, shared memory, block size, and occupancy
for all kernels. Reads all `params/*.json` automatically.
Output: `Toop_Bench/results/ncu/kernel_info_<timestamp>.csv`

```bash
python Toop_Bench/scripts/query_kernel_info.py
```

Dry run (print ncu commands without executing):
```bash
python Toop_Bench/scripts/query_kernel_info.py --dry-run
```

---

### NCU -- sweep

Sweeps `threads_per_block` for a single kernel, collecting the metrics defined in the kernel's params JSON. Each tpb value runs ncu once and collects all metrics in one pass.
Output: `Toop_Bench/results/<kernel>_sweep_<timestamp>.csv`

While running, each tpb prints a summary line:

```bash
--> tpb=  128  occupancy=32.12%  coalescing_ld=17.24%
```

Flat occupancy and coalescing across all tpb values means block size is not the bottleneck -- the issue is the memory access pattern in the kernel.
Varying coalescing across tpb values means block size is worth tuning.

To add or remove metrics, edit the `ncu.metrics` list in the params JSON.
To change the tpb sweep range, edit `sweep.threads_per_block` in the params JSON.


```bash
# k_solve_constraints_xpbd
python Toop_Bench/scripts/run_ncu_sweep.py \
    --params Toop_Bench/params/k_solve_constraints.json

# k_integrate
python Toop_Bench/scripts/run_ncu_sweep.py \
    --params Toop_Bench/params/k_integrate.json

# k_update_roots
python Toop_Bench/scripts/run_ncu_sweep.py \
    --params Toop_Bench/params/k_update_roots.json

# k_pack_positions_aos
python Toop_Bench/scripts/run_ncu_sweep.py \
    --params Toop_Bench/params/k_pack_positions.json

# Dry run
python Toop_Bench/scripts/run_ncu_sweep.py \
    --params Toop_Bench/params/k_solve_constraints.json \
    --dry-run

# Show all parameters and example JSON
python Toop_Bench/scripts/run_ncu_sweep.py --list-params
```

---

### Valgrind

CPU-side memory leak and invalid access detection. CUDA driver allocations are expected and are not real leaks.
Output: `Toop_Bench/results/valgrind/valgrind_<timestamp>.log`

```bash
python Toop_Bench/scripts/run_valgrind.py \
    --exe /app/x64/Release/Toop_Bench \
    --config /app/config.json
```

Check the summary at the bottom of the log:
```bash
tail -20 Toop_Bench/results/valgrind/valgrind_<timestamp>.log
```

Key line to check: `definitely lost: 0 bytes` means no real leaks.

---

### Perf

CPU-side sampling profiler. Captures kernel launch overhead and host-side hotspots in `HairSimulator::Step()`.

Perf runs on the EC2 host, not inside Docker, due to kernel version constraints. Run these steps from the host after entering the shell container.

Step 1 -- copy the binary from the container to the host:
```bash
docker cp <container_id>:/app/x64/Release/Toop_Bench ~/Toop_Bench
docker cp <container_id>:/app/config.json ~/config.json
```

Step 2 -- set perf permissions and run on the host:
```bash
sudo sysctl -w kernel.perf_event_paranoid=-1
sudo apt-get install -y linux-tools-aws
cd ~
sudo perf record -g -F 99 ./Toop_Bench --headless --config .
sudo perf report --stdio --no-children > perf_report.txt
head -60 perf_report.txt
```

Note: increase `capture_frames` in `config.json` to 3000 to reduce init overhead in the profile (otherwise CUDA driver init dominates).

### nsys

GPU timeline profiler. Shows per-kernel execution time and call counts. nsys 2024.2.0 is available inside the Docker container and is on PATH automatically (set in Dockerfile).

Output: `Toop_Bench/results/nsys/nsys_timing_<timestamp>.csv`
Also generates: `toop_profile_<timestamp>.nsys-rep` and `.sqlite`

```bash
python Toop_Bench/scripts/run_nsys.py \
    --exe /app/x64/Release/Toop_Bench \
    --config /app/config.json
```

Dry run:
```bash
python Toop_Bench/scripts/run_nsys.py \
    --exe /app/x64/Release/Toop_Bench \
    --config /app/config.json \
    --dry-run
```

Show all parameters:
```bash
python Toop_Bench/scripts/run_nsys.py --list-params
```

To open the visual timeline in Nsight Systems GUI on Windows, copy the `.nsys-rep` out of the container first:
```bash
docker cp <container_id>:/app/Toop_Bench/results/nsys/toop_profile_<timestamp>.nsys-rep \
    ./toop_profile.nsys-rep
```

---

### Parse results

Aggregates all sweep CSVs into one summary table.
Output: `Toop_Bench/results/summary_<timestamp>.csv`

```bash
python Toop_Bench/scripts/parse_results.py

# Custom input/output
python Toop_Bench/scripts/parse_results.py \
    --input Toop_Bench/results/ncu \
    --output Toop_Bench/results/my_summary.csv
```

---

## Tests

Open a shell and run the test binary directly:
```bash
docker compose --profile shell run shell
/app/x64/Release/Toop_Tests
```

Or use the test profile (non-interactive):
```bash
docker compose --profile test run tests
```

---

## Git

Branch strategy:
- `main` -- stable, all work merged here
- `feature/*` -- feature branches, merge to main with `--no-ff`

Skip worktree for `config.json` to avoid committing local param changes:
```bash
git update-index --skip-worktree config.json
```

Re-apply after cloning:
```bash
git update-index --skip-worktree config.json
```

---

## Config

`config.json` lives at the project root and is copied next to the binary at build time.
Pass the directory containing it to the binary with `--config <dir>`.

| Section        | Description                                      |
|----------------|--------------------------------------------------|
| `sim`          | Strand count, segments, compliance, damping      |
| `sphere`       | Physics parameters for the bouncing sphere       |
| `room`         | Bounding box and ground plane                    |
| `bald_patches` | Eye and top-of-head hair exclusion zones         |
| `render`       | Window size, vsync (interactive mode only)       |
| `profile`      | Warmup frames, capture frames, output directory  |