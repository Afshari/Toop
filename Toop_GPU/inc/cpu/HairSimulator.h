#pragma once
#include "Config.h"

namespace Toop {

    struct StepTimings
    {
        float sphere_collision_ms = 0.0f;
        float ground_collision_ms = 0.0f;
        float total_ms = 0.0f;
    };

    class HairSimulator
    {
    public:
        void Init(const SimConfig& config);
        StepTimings Step(float dt);
        void Shutdown();

        bool IsInitialized() const { return m_initialized; }
        int  GetTotalParticles() const { return m_total_particles; }

    private:
        void AllocateBuffers();
        void FreeBuffers();
        void UploadInitialPositions();

        // sim params - stored from config at Init time
        int   m_num_strands = 0;
        int   m_num_segments = 0;
        int   m_total_particles = 0;
        float m_segment_length = 0.0f;
        float m_gravity = 0.0f;
        float m_damping = 0.0f;
        float m_compliance = 0.0f;
        float m_sphere_radius = 0.0f;
        float m_sphere_cx = 0.0f;
        float m_sphere_cy = 0.0f;
        float m_sphere_cz = 0.0f;
        float m_ground_y = 0.0f;
        int   m_num_substeps = 0;
        int   m_threads_per_block = 128;
        bool  m_initialized = false;

        // GPU buffers - SoA layout
        float* m_d_pos_x = nullptr;
        float* m_d_pos_y = nullptr;
        float* m_d_pos_z = nullptr;
        float* m_d_prev_pos_x = nullptr;
        float* m_d_prev_pos_y = nullptr;
        float* m_d_prev_pos_z = nullptr;
        float* m_d_inv_mass = nullptr;
        float* m_d_rest_lengths = nullptr;
    };

} // namespace Toop