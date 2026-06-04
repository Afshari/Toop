#pragma once
#include <cuda_runtime.h>
#include <curand_kernel.h>

namespace Toop {

    // --------------------------------------------------------------------------------
    // Host-callable kernel launchers
    // All kernels live in HairSim.cu
    // --------------------------------------------------------------------------------

    void launch_solve_ground_collision(
        float* pos_y,
        const float* inv_mass,
        int   total_particles,
        float ground_y,
        int   threads_per_block);

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
        int    threads_per_block);

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
        int   threads_per_block);

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
        int   threads_per_block);

    void launch_solve_constraints_xpbd(
        float* pos_x,
        float* pos_y,
        float* pos_z,
        const float* inv_mass,
        const float* rest_lengths,
        float* lambdas,
        int   num_strands,
        int   num_segments,
        int   particles_per_strand,
        float compliance,
        float sub_dt,
        int   threads_per_block);

    void launch_init_rand_states(
        curandState* states,
        int          total_particles,
        unsigned long long seed,
        int          threads_per_block);

    void launch_translate_with_perturbation(
        float* pos_x,
        float* pos_y,
        float* pos_z,
        float* prev_pos_x,
        float* prev_pos_y,
        float* prev_pos_z,
        const float* inv_mass,
        curandState* states,
        int   total_particles,
        float delta_x,
        float delta_y,
        float delta_z,
        float noise_scale,
        int   threads_per_block);

} // namespace Toop