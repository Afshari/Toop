#include "cpu/HairSimulator.h"
#include "gpu/HairSim.cuh"
#include "gpu/CudaTimer.cuh"
#include "FibonacciSphere.h"
#include <cuda_runtime.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <stdexcept>

namespace Toop {

    // --------------------------------------------------------------------------------
    static void CheckCuda(cudaError_t err, const char* location)
    {
        if (err != cudaSuccess)
        {
            std::cerr << "[ERROR] CUDA error at " << location
                << ": " << cudaGetErrorString(err) << std::endl;
            throw std::runtime_error(cudaGetErrorString(err));
        }
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::Init(const SimConfig& config, const BaldPatchConfig& bald_patches)
    {
        m_num_strands = config.num_strands;
        m_num_segments = config.num_segments;
        m_total_particles = config.num_strands * (config.num_segments + 1);
        m_segment_length = config.segment_length;
        m_gravity = config.gravity;
        m_damping = config.damping;
        m_compliance = config.compliance;
        m_sphere_radius = config.sphere_radius;
        m_sphere_cx = config.sphere_center.x;
        m_sphere_cy = config.sphere_center.y;
        m_sphere_cz = config.sphere_center.z;
        m_num_substeps = config.num_substeps;

        AllocateBuffers();
        UploadRootDirs(config, bald_patches);
        InitRandStates();

        m_initialized = true;
        std::cout << "[INFO] HairSimulator initialized." << std::endl;
        std::cout << "[INFO] Total particles: " << m_total_particles << std::endl;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::AllocateBuffers()
    {
        size_t n = m_total_particles;
        size_t bytes = n * sizeof(float);

        CheckCuda(cudaMalloc(&m_d_pos_x, bytes), "cudaMalloc pos_x");
        CheckCuda(cudaMalloc(&m_d_pos_y, bytes), "cudaMalloc pos_y");
        CheckCuda(cudaMalloc(&m_d_pos_z, bytes), "cudaMalloc pos_z");
        CheckCuda(cudaMalloc(&m_d_prev_pos_x, bytes), "cudaMalloc prev_pos_x");
        CheckCuda(cudaMalloc(&m_d_prev_pos_y, bytes), "cudaMalloc prev_pos_y");
        CheckCuda(cudaMalloc(&m_d_prev_pos_z, bytes), "cudaMalloc prev_pos_z");
        CheckCuda(cudaMalloc(&m_d_inv_mass, bytes), "cudaMalloc inv_mass");
        CheckCuda(cudaMalloc(&m_d_rest_lengths,
            m_num_strands * m_num_segments * sizeof(float)), "cudaMalloc rest_lengths");
        CheckCuda(cudaMalloc(&m_d_root_dirs, m_num_strands * sizeof(float3)), "cudaMalloc root_dirs");
        CheckCuda(cudaMalloc(&m_d_lambdas,
            m_num_strands * m_num_segments * sizeof(float)), "cudaMalloc lambdas");
        CheckCuda(cudaMalloc(&m_d_rand_states,
            m_total_particles * sizeof(curandState)), "cudaMalloc rand_states");

        std::cout << "[INFO] GPU buffers allocated: "
            << (bytes * 7 + m_num_strands * m_num_segments * sizeof(float))
            / (1024 * 1024)
            << " MB" << std::endl;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::UploadInitialPositions(const std::vector<float3cpu>& root_dirs)
    {
        size_t n = m_total_particles;
        std::vector<float> h_pos_x(n, 0.0f);
        std::vector<float> h_pos_y(n, 0.0f);
        std::vector<float> h_pos_z(n, 0.0f);
        std::vector<float> h_inv_mass(n, 0.0f);
        std::vector<float> h_rest_lengths(m_num_strands * m_num_segments, m_segment_length);

        // placeholder - flat positions, proper Fibonacci init comes later
        int particles_per_strand = m_num_segments + 1;
        for (int s = 0; s < m_num_strands; s++)
        {
            // root position on sphere surface
            float rx = m_sphere_cx + root_dirs[s].x * m_sphere_radius;
            float ry = m_sphere_cy + root_dirs[s].y * m_sphere_radius;
            float rz = m_sphere_cz + root_dirs[s].z * m_sphere_radius;

            int base = s * particles_per_strand;

            for (int p = 0; p < particles_per_strand; p++)
            {
                // place particles along outward normal from root
                h_pos_x[base + p] = rx + root_dirs[s].x * p * m_segment_length;
                h_pos_y[base + p] = ry + root_dirs[s].y * p * m_segment_length;
                h_pos_z[base + p] = rz + root_dirs[s].z * p * m_segment_length;
                h_inv_mass[base + p] = (p == 0) ? 0.0f : 1.0f;
            }
        }

        size_t bytes = n * sizeof(float);
        CheckCuda(cudaMemcpy(m_d_pos_x, h_pos_x.data(), bytes, cudaMemcpyHostToDevice), "memcpy pos_x");
        CheckCuda(cudaMemcpy(m_d_pos_y, h_pos_y.data(), bytes, cudaMemcpyHostToDevice), "memcpy pos_y");
        CheckCuda(cudaMemcpy(m_d_pos_z, h_pos_z.data(), bytes, cudaMemcpyHostToDevice), "memcpy pos_z");
        CheckCuda(cudaMemcpy(m_d_prev_pos_x, h_pos_x.data(), bytes, cudaMemcpyHostToDevice), "memcpy prev_pos_x");
        CheckCuda(cudaMemcpy(m_d_prev_pos_y, h_pos_y.data(), bytes, cudaMemcpyHostToDevice), "memcpy prev_pos_y");
        CheckCuda(cudaMemcpy(m_d_prev_pos_z, h_pos_z.data(), bytes, cudaMemcpyHostToDevice), "memcpy prev_pos_z");
        CheckCuda(cudaMemcpy(m_d_inv_mass, h_inv_mass.data(), bytes, cudaMemcpyHostToDevice), "memcpy inv_mass");
        CheckCuda(cudaMemcpy(m_d_rest_lengths, h_rest_lengths.data(),
            m_num_strands * m_num_segments * sizeof(float),
            cudaMemcpyHostToDevice), "memcpy rest_lengths");
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::UploadRootDirs(
        const SimConfig& config,
        const BaldPatchConfig& bald_patches)
    {
        auto cpu_dirs = FibonacciSphere::Generate(
            m_num_strands, bald_patches);

        // upload initial positions BEFORE converting to GPU format
        UploadInitialPositions(cpu_dirs);

        // convert and upload root dirs to GPU
        std::vector<float3> gpu_dirs(m_num_strands);
        for (int i = 0; i < m_num_strands; i++)
        {
            gpu_dirs[i].x = cpu_dirs[i].x;
            gpu_dirs[i].y = cpu_dirs[i].y;
            gpu_dirs[i].z = cpu_dirs[i].z;
        }

        CheckCuda(cudaMemcpy(
            m_d_root_dirs,
            gpu_dirs.data(),
            m_num_strands * sizeof(float3),
            cudaMemcpyHostToDevice), "memcpy root_dirs");

        std::cout << "[INFO] Root dirs uploaded: "
            << m_num_strands << " strands." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::InitRandStates()
    {
        launch_init_rand_states(
            static_cast<curandState*>(m_d_rand_states),
            m_total_particles,
            1234ULL,
            m_threads_per_block);

        std::cout << "[INFO] curand states initialized." << std::endl;
    }

    // --------------------------------------------------------------------------------
    StepTimings HairSimulator::Step(float dt)
    {
        StepTimings timings;
        if (!m_initialized) return timings;

        float sub_dt = dt / m_num_substeps;

        CudaTimer timer_roots;
        CudaTimer timer_integrate;
        CudaTimer timer_sphere;
        CudaTimer timer_ground;
        CudaTimer timer_constraints;
        CudaTimer timer_total;

        timer_total.Start();

        for (int i = 0; i < m_num_substeps; i++)
        {
            timer_roots.Start();
            launch_update_roots(
                m_d_pos_x, m_d_pos_y, m_d_pos_z,
                m_d_prev_pos_x, m_d_prev_pos_y, m_d_prev_pos_z,
                static_cast<float3*>(m_d_root_dirs),
                m_num_strands,
                m_num_segments + 1,
                m_sphere_cx, m_sphere_cy, m_sphere_cz,
                m_sphere_radius,
                m_sphere_qx, m_sphere_qy, m_sphere_qz, m_sphere_qw,
                m_threads_per_block);
            timer_roots.Stop();
            timings.update_roots_ms += timer_roots.ElapsedMs();

            timer_integrate.Start();
            launch_integrate(
                m_d_pos_x, m_d_pos_y, m_d_pos_z,
                m_d_prev_pos_x, m_d_prev_pos_y, m_d_prev_pos_z,
                m_d_inv_mass,
                m_total_particles,
                m_gravity,
                m_damping,
                sub_dt,
                m_threads_per_block);
            timer_integrate.Stop();
            timings.integrate_ms += timer_integrate.ElapsedMs();

            // reset lambdas at start of each substep
            CheckCuda(cudaMemset(m_d_lambdas, 0,
                m_num_strands * m_num_segments * sizeof(float)),
                "cudaMemset lambdas");

            timer_constraints.Start();
            launch_solve_constraints_xpbd(
                m_d_pos_x, m_d_pos_y, m_d_pos_z,
                m_d_inv_mass,
                m_d_rest_lengths,
                m_d_lambdas,
                m_num_strands,
                m_num_segments,
                m_num_segments + 1,
                m_compliance,
                sub_dt,
                m_threads_per_block);
            timer_constraints.Stop();
            timings.constraints_ms += timer_constraints.ElapsedMs();

            timer_sphere.Start();
            launch_solve_sphere_collision(
                m_d_pos_x, m_d_pos_y, m_d_pos_z,
                m_d_inv_mass,
                m_total_particles,
                m_sphere_cx, m_sphere_cy, m_sphere_cz,
                m_sphere_radius,
                m_compliance,
                sub_dt,
                m_threads_per_block);
            timer_sphere.Stop();
            timings.sphere_collision_ms += timer_sphere.ElapsedMs();

            timer_ground.Start();
            launch_solve_ground_collision(
                m_d_pos_y,
                m_d_inv_mass,
                m_total_particles,
                m_ground_y,
                m_threads_per_block);
            timer_ground.Stop();
            timings.ground_collision_ms += timer_ground.ElapsedMs();
        }

        timer_total.Stop();
        timings.total_ms = timer_total.ElapsedMs();
        return timings;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::TranslateWithPerturbation(
        float delta_x, float delta_y, float delta_z,
        float noise_scale)
    {
        if (!m_initialized) return;

        launch_translate_with_perturbation(
            m_d_pos_x, m_d_pos_y, m_d_pos_z,
            m_d_prev_pos_x, m_d_prev_pos_y, m_d_prev_pos_z,
            m_d_inv_mass,
            static_cast<curandState*>(m_d_rand_states),
            m_total_particles,
            delta_x, delta_y, delta_z,
            noise_scale,
            m_threads_per_block);
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::SetSphereState(
        float px, float py, float pz,
        float qx, float qy, float qz, float qw)
    {
        m_sphere_cx = px;
        m_sphere_cy = py;
        m_sphere_cz = pz;
        m_sphere_qx = qx;
        m_sphere_qy = qy;
        m_sphere_qz = qz;
        m_sphere_qw = qw;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::Shutdown()
    {
        FreeBuffers();
        m_initialized = false;
        std::cout << "[INFO] HairSimulator shutdown." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::FreeBuffers()
    {
        cudaFree(m_d_pos_x);
        cudaFree(m_d_pos_y);
        cudaFree(m_d_pos_z);
        cudaFree(m_d_prev_pos_x);
        cudaFree(m_d_prev_pos_y);
        cudaFree(m_d_prev_pos_z);
        cudaFree(m_d_inv_mass);
        cudaFree(m_d_rest_lengths);
        cudaFree(m_d_root_dirs);
        cudaFree(m_d_lambdas);

        m_d_pos_x = nullptr;
        m_d_pos_y = nullptr;
        m_d_pos_z = nullptr;
        m_d_prev_pos_x = nullptr;
        m_d_prev_pos_y = nullptr;
        m_d_prev_pos_z = nullptr;
        m_d_inv_mass = nullptr;
        m_d_rest_lengths = nullptr;
        m_d_root_dirs = nullptr;
        m_d_lambdas = nullptr;
    }

    void HairSimulator::PackPositionsForRendering()
    {
        if (!m_initialized) return;
        if (!m_d_interop_aos)
        {
            std::cerr << "[WARN] PackPositionsForRendering: interop buffer is null." << std::endl;
            return;
        }

        launch_pack_positions_aos(
            m_d_pos_x, m_d_pos_y, m_d_pos_z,
            static_cast<float*>(m_d_interop_aos),
            m_total_particles,
            m_threads_per_block);
    }

} // namespace Toop