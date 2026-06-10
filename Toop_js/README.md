# Toop.js

Real-time interactive fur simulation in the browser. A furry sphere with eyes and personality — drag it, throw it, watch it react. Built with Three.js and GPGPU physics.

[![Live Demo](https://img.shields.io/badge/Live%20Demo-toopjs.vercel.app-black?style=for-the-badge)](https://toopjs.vercel.app)
[![YouTube](https://img.shields.io/badge/YouTube-Watch%20Demo-red?style=for-the-badge&logo=youtube)](https://youtube.com/shorts/UIwsClY8uwk)

[![Toop Demo](../docs/assets/Toop.js.gif)](https://toopjs.vercel.app)

---

## Key Features

- ~15,000 fur strands simulated in real-time using **Position-Based Dynamics (PBD/xPBD)**
- **GPGPU-based physics** — each strand computed in parallel on the GPU
- Fully interactive — drag and throw the sphere, eyes follow the mouse, character tilts toward you when idle
- Runs entirely in the **browser**, no installation needed
- **Three.js rendering** with custom vertex and fragment shaders
- Unlike noise-based animation, this project simulates **real physics**

---

## Technologies

- JavaScript
- GLSL
- Three.js
- GPGPU (GPU General Purpose Computing via fragment shaders)

---

## Technical Overview

The simulation runs almost entirely on the GPU. A **GPGPU fragment shader** computes the physics each frame and writes results to a texture. This texture is consumed by **custom vertex and fragment shaders** for rendering. Dynamic parameters are passed from CPU to GPU every frame. Only the initial strand setup runs on the CPU.

The sphere is fully interactive — physics handles collision response between the fur strands and the sphere surface, as well as ground collisions when the sphere is thrown.

---

## Performance

- **~15,000 strands** running at **40+ FPS** in the browser on GPU

---

## Project Structure

```text
index.html
src/
  main.js                  - entry point
  HairSimulation.js        - GPGPU simulation pipeline
  Sphere.js                - sphere physics, character behavior and interaction
  StrandGeometry.js        - strand geometry setup
  config.js                - simulation parameters
  shaders/
    integrate.glsl         - PBD integration step
    solve_constraints.glsl - constraint solver
    update_velocity.glsl   - velocity update
    sphere_collision.glsl  - xPBD sphere collision response
    ground_collision.glsl  - ground collision response
    translate_all.glsl     - strand translation during drag with perturbation
    strand_vert.glsl       - rendering vertex shader
    strand_frag.glsl       - rendering fragment shader
  utils/
    mathUtils.js           - math helpers
```

---

## How to Run

```bash
npm install
npm run dev
```

---

## Background & Credits

PBD concepts learned from the **[10 Minute Physics](https://www.youtube.com/@TenMinutePhysics)** YouTube channel.
Three.js skills learned from **[Three.js Journey](https://threejs-journey.com)**.
Implementation, GPGPU pipeline, and rendering architecture are original.