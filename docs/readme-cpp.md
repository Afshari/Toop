# Toop — C++ / CUDA Version

High-performance native implementation of the Toop fur simulation built with C++17 and CUDA 12.5.
Same physics as the [JavaScript version](../Toop_js/README.md), running on the GPU at full native speed.

---

## Overview

The C++ version reimplements the xPBD fur simulation as a native CUDA application.
Each hair strand is simulated in parallel on the GPU. The simulation runs headlessly
on AWS (Tesla T4) and interactively on desktop with OpenGL rendering.

---

## Features

- ~15,000 hair strands simulated in real-time using **xPBD (Extended Position-Based Dynamics)**
- Full **CUDA implementation** — all physics kernels run on the GPU
- **SoA (Structure of Arrays)** memory layout for GPU efficiency
- Interactive OpenGL rendering with drag, throw, and wind interaction
- Headless mode for server-side benchmarking and profiling
- Deployed and profiled on **AWS EC2 (Tesla T4)**
- Profiling infrastructure: ncu sweep scripts, valgrind, perf

---

## Targets

| Platform | GPU | Compute Capability | Mode |
|---|---|---|---|
| Windows (dev) | RTX 3060 Laptop | 8.6 | Interactive + headless |
| AWS EC2 | Tesla T4 | 7.5 | Headless only |
| Jetson Nano | Maxwell | 5.3 | Interactive + headless |

---

## Project Structure

```
Toop/                      # Entry point, AppRunner, InputHandler
Toop_CPU/                  # Config, CLParser, Camera, SpherePhysics, FibonacciSphere
Toop_GPU/                  # HairSimulator, CUDA kernels, Renderer, Window
Toop_Tests/                # GoogleTest unit tests
Toop_Bench/                # Profiling infrastructure
  scripts/                 # Python and shell profiling scripts
  params/                  # Per-kernel ncu sweep configs (JSON)
  results/                 # Generated output (gitignored)
cmake/                     # Shared CMake helpers
```

---

## CUDA Kernels

| Kernel | Description |
|---|---|
| `k_update_roots` | Strand roots follow sphere orientation via quaternion rotation |
| `k_integrate` | Implicit velocity integration with gravity, damping, and wind noise |
| `k_solve_constraints_xpbd` | Sequential Gauss-Seidel xPBD, one thread per strand |
| `k_solve_sphere_collision` | xPBD sphere push-out response |
| `k_solve_ground_collision` | Ground plane clamping |
| `k_translate_with_perturbation` | Drag translation with distance falloff and noise |
| `k_pack_positions_aos` | SoA to AoS conversion for OpenGL interop |
| `k_init_rand_states` | curand state initialization for wind noise |

---

## Build

### Windows (Visual Studio 2022)

Open `Toop.sln` and build in Release x64.

### Linux / Docker (AWS)

```bash
docker compose build
docker compose --profile shell run --privileged shell
```

See [docs/dev-guide.md](./dev-guide.md) for the full workflow.

---

## Dependencies

| Library | Purpose |
|---|---|
| CUDA 12.5 | GPU compute |
| curand | Wind noise random states |
| OpenGL / GLFW / GLAD | Interactive rendering (non-headless only) |
| GLM | Math (vectors, quaternions) |
| Boost.PropertyTree | JSON config parsing |
| Dear ImGui | Debug UI overlay (debug builds only) |
| GoogleTest | Unit tests |