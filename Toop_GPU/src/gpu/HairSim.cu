#include "gpu/HairSim.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace Toop {

    // --------------------------------------------------------------------------------
    // DEVICE KERNELS
    // --------------------------------------------------------------------------------

    __global__ void k_solve_ground_collision(
        float* __restrict__ pos_y,
        const float* __restrict__ inv_mass,
        int   total_particles,
        float ground_y)
    {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= total_particles) return;
        if (inv_mass[idx] == 0.0f)  return;   // skip roots
        if (pos_y[idx] < ground_y)
            pos_y[idx] = ground_y;
    }

    __global__ void k_solve_sphere_collision(
        float* __restrict__ pos_x,
        float* __restrict__ pos_y,
        float* __restrict__ pos_z,
        const float* __restrict__ inv_mass,
        int    total_particles,
        float  sphere_cx,
        float  sphere_cy,
        float  sphere_cz,
        float  sphere_radius,
        float  compliance,
        float  sub_dt)
    {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= total_particles) return;

        float w = inv_mass[idx];
        if (w == 0.0f) return;   // skip roots

        float dx = pos_x[idx] - sphere_cx;
        float dy = pos_y[idx] - sphere_cy;
        float dz = pos_z[idx] - sphere_cz;

        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist >= sphere_radius || dist < 1e-6f) return;

        float C = dist - sphere_radius;
        float alpha_tilde = compliance / (sub_dt * sub_dt);
        float d_lambda = -C / (w + alpha_tilde);

        float inv_dist = 1.0f / dist;
        pos_x[idx] += w * d_lambda * dx * inv_dist;
        pos_y[idx] += w * d_lambda * dy * inv_dist;
        pos_z[idx] += w * d_lambda * dz * inv_dist;
    }

    // --------------------------------------------------------------------------------
    // HOST LAUNCHERS
    // --------------------------------------------------------------------------------

    void launch_solve_ground_collision(
        float* pos_y,
        const float* inv_mass,
        int   total_particles,
        float ground_y,
        int   threads_per_block)
    {
        int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
        k_solve_ground_collision << <blocks, threads_per_block >> > (
            pos_y, inv_mass, total_particles, ground_y);
    }

    void launch_solve_sphere_collision(
        float* pos_x,
        float* pos_y,
        float* pos_z,
        const float* inv_mass,
        int    total_particles,
        float  sphere_cx,
        float  sphere_cy,
        float  sphere_cz,
        float  sphere_radius,
        float  compliance,
        float  sub_dt,
        int    threads_per_block)
    {
        int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
        k_solve_sphere_collision << <blocks, threads_per_block >> > (
            pos_x, pos_y, pos_z, inv_mass,
            total_particles,
            sphere_cx, sphere_cy, sphere_cz,
            sphere_radius, compliance, sub_dt);
    }

} // namespace Toop