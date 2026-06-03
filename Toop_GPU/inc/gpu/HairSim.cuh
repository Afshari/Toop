#pragma once
#include <cuda_runtime.h>

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

} // namespace Toop