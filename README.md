# Toop

**Toop** means *ball* in Farsi. It is a real-time interactive fur simulation — a furry sphere with eyes, personality, and physics. Drag it, throw it, watch it react.

[![Live Demo](https://img.shields.io/badge/Live%20Demo-toopjs.vercel.app-black?style=for-the-badge)](https://toopjs.vercel.app)
[![YouTube](https://img.shields.io/badge/YouTube-Watch%20Demo-red?style=for-the-badge&logo=youtube)](https://www.youtube.com/shorts/qccjEpDzxB0)

---

[![Toop Demo](docs/assets/Toop.js.gif)](https://toopjs.vercel.app)

---

## Project Structure

This repository contains two implementations of the same simulation:

| Directory | Description |
|-----------|-------------|
| [`Toop_js/`](./Toop_js/README.md) | JavaScript version — runs in the browser using Three.js and GPGPU. Live demo available. |
| [`docs/readme-cpp.md`](./docs/readme-cpp.md) | C++ / CUDA version — high-performance native implementation with GPU profiling infrastructure. |
| [`docs/perf-cuda-results.md`](./docs/perf-cuda-results.md) | CUDA profiling results — kernel timing, memory coalescing analysis, optimization notes. |
| [`docs/dev-guide.md`](./docs/dev-guide.md) | Developer guide — build, AWS workflow, profiling commands. |

See the [Toop.js README](./Toop_js/README.md) for details on the JavaScript version, how to run it locally, and technical implementation notes.

---

## CUDA Profiling Highlights

Profiled on AWS EC2 Tesla T4 (CC 7.5) using ncu and nsys.

| Kernel | GPU time share | Coalescing (before) | Coalescing (after) |
|---|---|---|---|
| `k_solve_constraints_xpbd` | 41% | 17% | 54% |
| `k_integrate` | 23% | 100% | 100% |
| `k_solve_sphere_collision` | 15% | -- | -- |
| `k_update_roots` | 8% | 33% | 33% |

Memory layout refactored from strand-major SoA to particle-major layout, improving load coalescing on the primary kernel from 17% to 54%.

See [docs/perf-cuda-results.md](./docs/perf-cuda-results.md) for full results.

---

## Credits

- **[10 Minute Physics](https://www.youtube.com/@TenMinutePhysics)** — I learned about Extended Position-Based Dynamics (XPBD) from this YouTube channel and used it as the foundation for the physics simulation in this project.

- **[Three.js Journey](https://threejs-journey.com)** — I learned Three.js from this excellent learning platform, which was essential for building the browser-based version.