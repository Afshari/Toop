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

    __global__ void k_integrate(
        float* __restrict__ pos_x,
        float* __restrict__ pos_y,
        float* __restrict__ pos_z,
        float* __restrict__ prev_pos_x,
        float* __restrict__ prev_pos_y,
        float* __restrict__ prev_pos_z,
        const float* __restrict__ inv_mass,
        int   total_particles,
        float gravity_y,
        float damping,
        float dt)
    {
        int idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= total_particles) return;
        if (inv_mass[idx] == 0.0f) return;

        float vx = (pos_x[idx] - prev_pos_x[idx]) / dt;
        float vy = (pos_y[idx] - prev_pos_y[idx]) / dt;
        float vz = (pos_z[idx] - prev_pos_z[idx]) / dt;

        vy += gravity_y * dt;

        vx *= damping;
        vy *= damping;
        vz *= damping;

        prev_pos_x[idx] = pos_x[idx];
        prev_pos_y[idx] = pos_y[idx];
        prev_pos_z[idx] = pos_z[idx];

        pos_x[idx] += vx * dt;
        pos_y[idx] += vy * dt;
        pos_z[idx] += vz * dt;
    }

    __global__ void k_update_roots(
        float* __restrict__ pos_x,
        float* __restrict__ pos_y,
        float* __restrict__ pos_z,
        float* __restrict__ prev_pos_x,
        float* __restrict__ prev_pos_y,
        float* __restrict__ prev_pos_z,
        const float3* __restrict__ root_dirs,
        int   num_strands,
        int   particles_per_strand,
        float sphere_cx,
        float sphere_cy,
        float sphere_cz,
        float sphere_radius,
        float qx, float qy, float qz, float qw)
    {
        int sid = blockIdx.x * blockDim.x + threadIdx.x;
        if (sid >= num_strands) return;

        // rotate root_dir by quaternion (Rodrigues formula)
        float3 v = root_dirs[sid];

        float cx = qy * v.z - qz * v.y;
        float cy = qz * v.x - qx * v.z;
        float cz = qx * v.y - qy * v.x;

        float rx = v.x + 2.0f * qw * cx + 2.0f * (qy * cz - qz * cy);
        float ry = v.y + 2.0f * qw * cy + 2.0f * (qz * cx - qx * cz);
        float rz = v.z + 2.0f * qw * cz + 2.0f * (qx * cy - qy * cx);

        float root_x = sphere_cx + rx * sphere_radius;
        float root_y = sphere_cy + ry * sphere_radius;
        float root_z = sphere_cz + rz * sphere_radius;

        int root_idx = sid * particles_per_strand;

        // update both pos and prev_pos to avoid velocity spike at root
        pos_x[root_idx] = root_x;
        pos_y[root_idx] = root_y;
        pos_z[root_idx] = root_z;

        prev_pos_x[root_idx] = root_x;
        prev_pos_y[root_idx] = root_y;
        prev_pos_z[root_idx] = root_z;
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

    void launch_integrate(
        float* pos_x,
        float* pos_y,
        float* pos_z,
        float* prev_pos_x,
        float* prev_pos_y,
        float* prev_pos_z,
        const float* inv_mass,
        int   total_particles,
        float gravity_y,
        float damping,
        float dt,
        int   threads_per_block)
    {
        int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
        k_integrate << <blocks, threads_per_block >> > (
            pos_x, pos_y, pos_z,
            prev_pos_x, prev_pos_y, prev_pos_z,
            inv_mass,
            total_particles,
            gravity_y, damping, dt);
    }

    void launch_update_roots(
        float* pos_x,
        float* pos_y,
        float* pos_z,
        float* prev_pos_x,
        float* prev_pos_y,
        float* prev_pos_z,
        const float3* root_dirs,
        int   num_strands,
        int   particles_per_strand,
        float sphere_cx,
        float sphere_cy,
        float sphere_cz,
        float sphere_radius,
        float qx, float qy, float qz, float qw,
        int   threads_per_block)
    {
        int blocks = (num_strands + threads_per_block - 1) / threads_per_block;
        k_update_roots << <blocks, threads_per_block >> > (
            pos_x, pos_y, pos_z,
            prev_pos_x, prev_pos_y, prev_pos_z,
            root_dirs,
            num_strands,
            particles_per_strand,
            sphere_cx, sphere_cy, sphere_cz,
            sphere_radius,
            qx, qy, qz, qw);
    }

} // namespace Toop