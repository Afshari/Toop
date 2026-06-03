#include "cpu/HairSimulator.h"
#include "gpu/HairSim.cuh"
#include "gpu/CudaTimer.cuh"
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
    void HairSimulator::Init(const SimConfig& config)
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
        UploadInitialPositions();

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

        std::cout << "[INFO] GPU buffers allocated: "
            << (bytes * 7 + m_num_strands * m_num_segments * sizeof(float))
            / (1024 * 1024)
            << " MB" << std::endl;
    }

    // --------------------------------------------------------------------------------
    void HairSimulator::UploadInitialPositions()
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
            int base = s * particles_per_strand;
            for (int p = 0; p < particles_per_strand; p++)
            {
                h_pos_x[base + p] = 0.0f;
                h_pos_y[base + p] = m_sphere_cy + m_sphere_radius
                    + p * m_segment_length;
                h_pos_z[base + p] = 0.0f;
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
    StepTimings HairSimulator::Step(float dt)
    {
        StepTimings timings;
        if (!m_initialized) return timings;

        float sub_dt = dt / m_num_substeps;

        CudaTimer timer_sphere;
        CudaTimer timer_ground;
        CudaTimer timer_total;

        timer_total.Start();

        for (int i = 0; i < m_num_substeps; i++)
        {
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

        m_d_pos_x = nullptr;
        m_d_pos_y = nullptr;
        m_d_pos_z = nullptr;
        m_d_prev_pos_x = nullptr;
        m_d_prev_pos_y = nullptr;
        m_d_prev_pos_z = nullptr;
        m_d_inv_mass = nullptr;
        m_d_rest_lengths = nullptr;
    }

} // namespace Toop