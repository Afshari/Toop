#pragma once
#ifndef TOOP_HEADLESS
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "ShaderProgram.h"
#include "Config.h"

namespace Toop {

    class HairSimulator;

    class Renderer
    {
    public:
        void Init(const Config& config, HairSimulator& sim);
        void Render(
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& sphere_pos,
            const glm::quat& sphere_orientation,
            float sphere_radius,
            const BaldPatchConfig& bald_patches,
            const glm::vec3& mouse_ray_origin,
            const glm::vec3& mouse_ray_dir,
            const glm::vec3& camera_forward);
        void Shutdown();

        float* GetInteropPtr() const { return m_interop_ptr; }
        void   MapInterop() { MapInteropBuffer(); }
        void   UnmapInterop() { UnmapInteropBuffer(); }

        bool IsInitialized() const { return m_initialized; }
        glm::vec3 GetEyeWorldPos(
            int                    eye_index,
            const glm::vec3& sphere_pos,
            const glm::quat& sphere_orientation,
            float                  sphere_radius,
            const BaldPatchConfig& bald_patches) const;

    private:
        void InitHairBuffers(int total_particles, int num_segments, int num_strands);
        void InitSphereBuffers(float radius);
        void InitRoomBuffers(const RoomConfig& room);
        void InitGroundBuffers(const RoomConfig& room);

        void RenderHair(const glm::mat4& view, const glm::mat4& proj);
        void RenderSphere(const glm::mat4& view, const glm::mat4& proj,
            const glm::vec3& pos, float radius);
        void RenderRoom(const glm::mat4& view, const glm::mat4& proj);
        void RenderGround(const glm::mat4& view, const glm::mat4& proj);

        void RenderEyes(
            const glm::mat4& view,
            const glm::mat4& proj,
            const glm::vec3& sphere_pos,
            const glm::quat& sphere_orientation,
            float sphere_radius,
            const BaldPatchConfig& bald_patches,
            const glm::vec3& mouse_ray_origin,
            const glm::vec3& mouse_ray_dir,
            const glm::vec3& camera_forward);

        void MapInteropBuffer();
        void UnmapInteropBuffer();

        // hair
        GLuint m_hair_vao = 0;
        GLuint m_hair_vbo = 0;
        GLuint m_hair_ebo = 0;
        void* m_cuda_vbo_resource = nullptr;
        int    m_hair_index_count = 0;

        // sphere
        GLuint m_sphere_vao = 0;
        GLuint m_sphere_vbo = 0;
        GLuint m_sphere_ebo = 0;
        int    m_sphere_index_count = 0;

        // room
        GLuint m_room_vao = 0;
        GLuint m_room_vbo = 0;

        // ground
        GLuint m_ground_vao = 0;
        GLuint m_ground_vbo = 0;

        // shaders
        ShaderProgram* m_hair_shader = nullptr;
        ShaderProgram* m_sphere_shader = nullptr;
        ShaderProgram* m_room_shader = nullptr;

        int   m_total_particles = 0;
        bool  m_initialized = false;

        float* m_interop_ptr = nullptr;
    };

} // namespace Toop
#endif // TOOP_HEADLESS
