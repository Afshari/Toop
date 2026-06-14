#pragma once
#include "Config.h"
#include "FibonacciSphere.h"

namespace Toop {

    struct StepTimings
    {
        float sphere_collision_ms = 0.0f;
        float ground_collision_ms = 0.0f;
        float integrate_ms = 0.0f;
        float update_roots_ms = 0.0f;
        float constraints_ms = 0.0f;
        float pack_aos_ms = 0.0f;
        float update_velocity_ms = 0.0f;
        float total_ms = 0.0f;
    };

    class HairSimulator
    {
    public:
        void Init(const SimConfig& sim, const BaldPatchConfig& bald_patches);
        StepTimings Step(float dt, bool is_dragging = false,
            float delta_x = 0.0f,
            float delta_y = 0.0f,
            float delta_z = 0.0f);
        void TranslateWithPerturbation(float delta_x, float delta_y, float delta_z, float drag_speed);
        void Shutdown();

        bool IsInitialized() const { return m_initialized; }
        int  GetTotalParticles() const { return m_total_particles; }
        void SetSphereState(float px, float py, float pz, float qx, float qy, float qz, float qw);

        void SetInteropBuffer(void* cuda_ptr) { m_d_interop_aos = cuda_ptr; }
        void PackPositionsForRendering();

        void SetWind(float wx, float wy, float wz) {
            m_wind_x = wx;
            m_wind_y = wy;
            m_wind_z = wz;
        }
        void ApplyDragVelocity(
            float vel_x, float vel_y, float vel_z,
            float scale);
        void AddTime(float dt) { m_time += dt; }

        void SetNumSubsteps(int n) { m_num_substeps = n; }
        void SetCompliance(float c) { m_compliance = c; }
        void SetDamping(float d) { m_damping = d; }
        void SetGravity(float g) { m_gravity = g; }

    private:
        void AllocateBuffers();
        void FreeBuffers();
        void UploadInitialPositions(const std::vector<float3cpu>& root_dirs);
        void UploadRootDirs(const SimConfig& config, const BaldPatchConfig& bald_patches);
        void InitRandStates();

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
        void* m_d_root_dirs = nullptr;

        float m_sphere_qx = 0.0f;
        float m_sphere_qy = 0.0f;
        float m_sphere_qz = 0.0f;
        float m_sphere_qw = 1.0f;

        float* m_d_lambdas = nullptr;
        void* m_d_rand_states = nullptr;

        void* m_d_interop_aos = nullptr;

        float m_wind_x = 0.0f;
        float m_wind_y = 0.0f;
        float m_wind_z = 0.0f;
        float m_time = 0.0f;
    };

} // namespace Toop