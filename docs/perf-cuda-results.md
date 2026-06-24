# Toop -- CUDA Profiling Results

Profiling results for the C++ / CUDA implementation of Toop.
Platform: AWS EC2 g4dn.xlarge, Tesla T4 (CC 7.5), CUDA 12.5.
Tools: ncu (NVIDIA Nsight Compute), nsys (NVIDIA Nsight Systems).

---

## Contents

- [Toop -- CUDA Profiling Results](#toop----cuda-profiling-results)
  - [Contents](#contents)
  - [Platform](#platform)
  - [Kernel Overview](#kernel-overview)
  - [Kernel Timing](#kernel-timing)
  - [Memory Coalescing Sweep](#memory-coalescing-sweep)
    - [Before optimization (SoA strand-major layout)](#before-optimization-soa-strand-major-layout)
    - [After optimization (particle-major layout)](#after-optimization-particle-major-layout)
  - [Occupancy](#occupancy)
  - [Optimization: Particle-Major Memory Layout](#optimization-particle-major-memory-layout)
    - [Problem](#problem)
    - [Fix](#fix)
    - [Result](#result)
  - [Valgrind](#valgrind)

---

## Platform

| Property | Value |
|---|---|
| Instance | AWS EC2 g4dn.xlarge |
| GPU | Tesla T4 |
| Compute Capability | 7.5 |
| CUDA | 12.5 |
| Driver | 580.159.03 |
| ncu | NVIDIA Nsight Compute 2024.2.0 |
| nsys | NVIDIA Nsight Systems 2024.2.0 |

---

## Kernel Overview

| Kernel | Description | Threads/Block |
|---|---|---|
| `k_solve_constraints_xpbd` | Sequential Gauss-Seidel xPBD, one thread per strand | 128 |
| `k_integrate` | Implicit velocity integration with gravity, damping, wind | 128 |
| `k_solve_sphere_collision` | xPBD sphere push-out response | 128 |
| `k_solve_ground_collision` | Ground plane clamping | 128 |
| `k_update_roots` | Strand roots follow sphere orientation via quaternion | 128 |
| `k_init_rand_states` | curand state initialization (once at startup) | 128 |
| `k_pack_positions_aos` | SoA to AoS conversion for OpenGL interop (not called headless) | 128 |

---

## Kernel Timing

Captured with nsys. 300 frames, 15 substeps per frame.

| Kernel | Total ms | Calls | Avg us | % of GPU time |
|---|---|---|---|---|
| `k_solve_constraints_xpbd` | 97.0 | 10800 | 8.98 | 41% |
| `k_integrate` | 54.8 | 10800 | 5.08 | 23% |
| `k_solve_sphere_collision` | 34.5 | 10800 | 3.19 | 15% |
| `k_solve_ground_collision` | 29.3 | 10800 | 2.71 | 12% |
| `k_update_roots` | 19.6 | 10800 | 1.82 | 8% |
| `k_init_rand_states` | 1.75 | 1 | 1751 | 1% |
| **Total** | **237 ms** | | | |

Total GPU time per frame: ~0.79ms. Headless avg frame time: ~2.5ms.
The remaining ~1.7ms per frame is kernel launch overhead, CUDA synchronization, and CPU-side work in `HairSimulator::Step()`.

---

## Memory Coalescing Sweep

Captured with ncu. Metric: `smsp__sass_average_data_bytes_per_sector_mem_global_op_ld.pct`.
Sweep over `threads_per_block` in [64, 128, 256, 512].

### Before optimization (SoA strand-major layout)

| Kernel | tpb | coalescing_ld | occupancy |
|---|---|---|---|
| `k_solve_constraints_xpbd` | 64-512 | 17.24% | ~32% |
| `k_update_roots` | 64-512 | 33.33% | ~28% |
| `k_integrate` | 64-512 | 99.99% | ~80% |

### After optimization (particle-major layout)

| Kernel | tpb | coalescing_ld | occupancy |
|---|---|---|---|
| `k_solve_constraints_xpbd` | 64-512 | **53.51%** | ~33% |
| `k_update_roots` | 64-512 | 33.33% | ~33% |
| `k_integrate` | 64-512 | 99.99% | ~80% |

Coalescing is flat across all tpb values for all kernels -- block size is not the bottleneck. The memory access pattern is the determining factor.

---

## Occupancy

Captured with ncu. Metric: `sm__warps_active.avg.pct_of_peak_sustained_active`.

| Kernel | Registers/thread | Theoretical | Achieved |
|---|---|---|---|
| `k_integrate` | 27 | 100% | 84% |
| `k_solve_constraints_xpbd` | 47 | 100% | 33% |
| `k_update_roots` | 22 | 100% | 33% |

`k_solve_constraints_xpbd` achieved occupancy (33%) is limited by the sequential Gauss-Seidel algorithm -- each constraint depends on the previous one, so threads cannot fully overlap their work. This is an algorithmic limit, not a register pressure or memory layout issue.

`k_update_roots` achieved occupancy (33%) is limited by the `float3` root direction reads -- 12 bytes per thread gives 33% sector utilization, which is a structural characteristic of reading packed float3 data.

---

## Optimization: Particle-Major Memory Layout

### Problem

The original SoA layout stored positions as separate `pos_x[]`, `pos_y[]`, `pos_z[]` arrays indexed strand-major: `particle_idx = strand * particles_per_strand + segment`.

In `k_solve_constraints_xpbd`, adjacent threads in a warp access memory with a stride of `particles_per_strand` (6) floats. Each 128-byte cache line fetch retrieved only ~22 bytes of useful data -- 17% coalescing efficiency.

### Fix

Changed to particle-major indexing: `particle_idx = segment * num_strands + strand`.

Adjacent threads now read adjacent memory locations. The constraint kernel reads `id0 = i * num_strands + sid` and `id1 = (i+1) * num_strands + sid`, where adjacent `sid` values are contiguous in memory.

### Result

| Metric | Before | After | Change |
|---|---|---|---|
| `k_solve_constraints_xpbd` coalescing | 17.24% | 53.51% | +36 points |
| `k_integrate` coalescing | 99.99% | 99.99% | unchanged |
| `k_update_roots` coalescing | 33.33% | 33.33% | unchanged |

The 53.51% result is at the theoretical maximum for this kernel's access pattern -- the constraint loop reads two particles per iteration (`id0`
and `id1`) that are `num_strands` floats apart, limiting peak coalescing to approximately 50%.

---

## Valgrind

CPU-side memory check. Result: no leaks in application code.

```
definitely lost:  0 bytes in 0 blocks
indirectly lost:  0 bytes in 0 blocks
possibly lost:    23,464 bytes in 185 blocks  (CUDA driver internals)
still reachable:  21 MB                       (CUDA driver internals)
```

All reported allocations trace back to `libcuda.so` driver internals (`cuInit`, `cuDevicePrimaryCtxRetain`) -- expected behavior for any CUDA application. No leaks in `HairSimulator`, `Config`, or any application-level code.