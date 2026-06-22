#include "cpu/Renderer.h"
#include "cpu/HairSimulator.h"
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <windows.h>
#include <filesystem>
#include "RayUtils.h"

namespace Toop {

    static constexpr float kPi = 3.14159265358979323846f;

    // --------------------------------------------------------------------------------
    static void CheckCudaInterop(cudaError_t err, const char* location)
    {
        if (err != cudaSuccess)
        {
            std::cerr << "[ERROR] CUDA interop error at " << location
                << ": " << cudaGetErrorString(err) << std::endl;
            throw std::runtime_error(cudaGetErrorString(err));
        }
    }

    static std::string ShaderPath(const std::string& filename)
    {
        char exe_path[MAX_PATH];
        GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
        std::filesystem::path dir =
            std::filesystem::path(exe_path).parent_path();
        return (dir / filename).string();
    }

    static glm::vec3 QuatRotate(const glm::quat& q, const glm::vec3& v)
    {
        return q * v;
    }

    // --------------------------------------------------------------------------------
    void Renderer::Init(const Config& config, HairSimulator& sim)
    {
        int total = sim.GetTotalParticles();
        int segments = config.sim.num_segments;
        int strands = config.sim.num_strands;

        m_total_particles = total;

        InitHairBuffers(total, segments, strands);
        InitSphereBuffers(config.sim.sphere_radius);
        InitRoomBuffers(config.room);
        InitGroundBuffers(config.room);

        // shaders
        m_hair_shader = new ShaderProgram(
            ShaderPath("shaders/hair_vert.glsl"),
            ShaderPath("shaders/hair_frag.glsl"));
        m_sphere_shader = new ShaderProgram(
            ShaderPath("shaders/sphere_vert.glsl"),
            ShaderPath("shaders/sphere_frag.glsl"));
        m_room_shader = new ShaderProgram(
            ShaderPath("shaders/room_vert.glsl"),
            ShaderPath("shaders/room_frag.glsl"));

        m_initialized = true;
        std::cout << "[INFO] Renderer initialized." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void Renderer::InitHairBuffers(
        int total_particles,
        int num_segments,
        int num_strands)
    {
        // build index buffer - each strand is num_segments lines
        // line strip per strand: p0-p1, p1-p2, p2-p3, ...
        std::vector<unsigned int> indices;
        int particles_per_strand = num_segments + 1;
        indices.reserve(num_strands * num_segments * 2);

        for (int s = 0; s < num_strands; s++)
        {
            int base = s * particles_per_strand;
            for (int i = 0; i < num_segments; i++)
            {
                indices.push_back(base + i);
                indices.push_back(base + i + 1);
            }
        }
        m_hair_index_count = (int)indices.size();

        glGenVertexArrays(1, &m_hair_vao);
        glGenBuffers(1, &m_hair_vbo);
        glGenBuffers(1, &m_hair_ebo);

        glBindVertexArray(m_hair_vao);

        // VBO - positions as AoS float3, size = total_particles * 3 floats
        glBindBuffer(GL_ARRAY_BUFFER, m_hair_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            total_particles * 3 * sizeof(float),
            nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // EBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hair_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);

        // ensure CUDA uses the same device as OpenGL
        unsigned int device_count = 0;
        int cuda_devices[1] = { 0 };
        cudaGLGetDevices(&device_count, cuda_devices, 1, cudaGLDeviceListAll);
        cudaSetDevice(cuda_devices[0]);

        std::cout << "[INFO] CUDA/GL interop device: " << cuda_devices[0] << std::endl;

        // register VBO with CUDA
        CheckCudaInterop(
            cudaGraphicsGLRegisterBuffer(
                reinterpret_cast<cudaGraphicsResource**>(&m_cuda_vbo_resource),
                m_hair_vbo,
                cudaGraphicsMapFlagsWriteDiscard),
            "cudaGraphicsGLRegisterBuffer");

        std::cout << "[INFO] Hair buffers initialized. Indices: "
            << m_hair_index_count << std::endl;
    }

    // --------------------------------------------------------------------------------
    void Renderer::InitSphereBuffers(float radius)
    {
        // UV sphere generation
        const int stacks = 16;
        const int slices = 16;

        std::vector<float>        verts;
        std::vector<unsigned int> indices;

        for (int i = 0; i <= stacks; i++)
        {
            float phi = (float)kPi * i / stacks;
            for (int j = 0; j <= slices; j++)
            {
                float theta = 2.0f * (float)kPi * j / slices;
                verts.push_back(radius * sinf(phi) * cosf(theta));
                verts.push_back(radius * cosf(phi));
                verts.push_back(radius * sinf(phi) * sinf(theta));
            }
        }

        for (int i = 0; i < stacks; i++)
        {
            for (int j = 0; j < slices; j++)
            {
                int a = i * (slices + 1) + j;
                int b = a + slices + 1;
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
                indices.push_back(b);
                indices.push_back(b + 1);
                indices.push_back(a + 1);
            }
        }
        m_sphere_index_count = (int)indices.size();

        glGenVertexArrays(1, &m_sphere_vao);
        glGenBuffers(1, &m_sphere_vbo);
        glGenBuffers(1, &m_sphere_ebo);

        glBindVertexArray(m_sphere_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_sphere_vbo);
        glBufferData(GL_ARRAY_BUFFER,
            verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphere_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned int),
            indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    // --------------------------------------------------------------------------------
    void Renderer::InitRoomBuffers(const RoomConfig& room)
    {
        float x0 = room.min.x, y0 = room.min.y, z0 = room.min.z;
        float x1 = room.max.x, y1 = room.max.y, z1 = room.max.z;

        // 8 corners of the box
        float verts[] = {
            x0,y0,z0,  x1,y0,z0,  x1,y1,z0,  x0,y1,z0,
            x0,y0,z1,  x1,y0,z1,  x1,y1,z1,  x0,y1,z1
        };

        // 12 edges as line pairs
        unsigned int indices[] = {
            0,1, 1,2, 2,3, 3,0,   // front face
            4,5, 5,6, 6,7, 7,4,   // back face
            0,4, 1,5, 2,6, 3,7    // connecting edges
        };

        glGenVertexArrays(1, &m_room_vao);
        glGenBuffers(1, &m_room_vbo);

        glBindVertexArray(m_room_vao);

        GLuint ebo;
        glGenBuffers(1, &ebo);

        glBindBuffer(GL_ARRAY_BUFFER, m_room_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    // --------------------------------------------------------------------------------
    void Renderer::InitGroundBuffers(const RoomConfig& room)
    {
        float y = room.ground_y;
        float x0 = room.min.x, z0 = room.min.z;
        float x1 = room.max.x, z1 = room.max.z;

        float verts[] = {
            x0, y, z0,
            x1, y, z0,
            x1, y, z1,
            x0, y, z1
        };

        unsigned int indices[] = { 0,1,2, 2,3,0 };

        glGenVertexArrays(1, &m_ground_vao);
        glGenBuffers(1, &m_ground_vbo);

        glBindVertexArray(m_ground_vao);

        GLuint ebo;
        glGenBuffers(1, &ebo);

        glBindBuffer(GL_ARRAY_BUFFER, m_ground_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    // --------------------------------------------------------------------------------
    void Renderer::MapInteropBuffer()
    {
        size_t size = 0;

        CheckCudaInterop(
            cudaGraphicsMapResources(
                1,
                reinterpret_cast<cudaGraphicsResource**>(&m_cuda_vbo_resource), 0),
            "cudaGraphicsMapResources");

        CheckCudaInterop(
            cudaGraphicsResourceGetMappedPointer(
                (void**)&m_interop_ptr, &size,
                reinterpret_cast<cudaGraphicsResource*>(m_cuda_vbo_resource)),
            "cudaGraphicsResourceGetMappedPointer");
    }

    // --------------------------------------------------------------------------------
    void Renderer::UnmapInteropBuffer()
    {
        CheckCudaInterop(
            cudaGraphicsUnmapResources(
                1,
                reinterpret_cast<cudaGraphicsResource**>(&m_cuda_vbo_resource), 0),
            "cudaGraphicsUnmapResources");
        m_interop_ptr = nullptr;
    }

    // --------------------------------------------------------------------------------
    void Renderer::Render(
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& sphere_pos,
        const glm::quat& sphere_orientation,
        float sphere_radius,
        const BaldPatchConfig& bald_patches,
        const glm::vec3& mouse_ray_origin,
        const glm::vec3& mouse_ray_dir,
        const glm::vec3& camera_forward,
        bool  frozen)
    {
        if (!m_initialized) return;

        RenderGround(view, proj);
        RenderRoom(view, proj);
        RenderSphere(view, proj, sphere_pos, sphere_orientation, sphere_radius);
        RenderEyes(view, proj, sphere_pos, sphere_orientation,
            sphere_radius, bald_patches,
            mouse_ray_origin, mouse_ray_dir, camera_forward, frozen);
        RenderHair(view, proj);
    }

    // --------------------------------------------------------------------------------
    void Renderer::RenderHair(
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        m_hair_shader->Bind();
        m_hair_shader->SetMat4("uView", glm::value_ptr(view));
        m_hair_shader->SetMat4("uProj", glm::value_ptr(proj));
        m_hair_shader->SetVec3("uColor", 1.0f, 0.5f, 0.5f);

        glBindVertexArray(m_hair_vao);
        glDrawElements(GL_LINES, m_hair_index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_hair_shader->Unbind();
    }

    // --------------------------------------------------------------------------------
    void Renderer::RenderSphere(
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& pos,
        const glm::quat& orientation,
        float            /*radius*/)
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos)
            * glm::mat4_cast(orientation);

        m_sphere_shader->Bind();
        m_sphere_shader->SetMat4("uModel", glm::value_ptr(model));
        m_sphere_shader->SetMat4("uView", glm::value_ptr(view));
        m_sphere_shader->SetMat4("uProj", glm::value_ptr(proj));
        m_sphere_shader->SetVec3("uColor", 0.9f, 0.15f, 0.15f);

        glBindVertexArray(m_sphere_vao);
        glDrawElements(GL_TRIANGLES, m_sphere_index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_sphere_shader->Unbind();
    }

    // --------------------------------------------------------------------------------
    void Renderer::RenderRoom(
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        m_room_shader->Bind();
        m_room_shader->SetMat4("uView", glm::value_ptr(view));
        m_room_shader->SetMat4("uProj", glm::value_ptr(proj));
        m_room_shader->SetVec3("uColor", 0.3f, 0.3f, 0.3f);

        glBindVertexArray(m_room_vao);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_room_shader->Unbind();
    }

    // --------------------------------------------------------------------------------
    void Renderer::RenderGround(
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        m_room_shader->Bind();
        m_room_shader->SetMat4("uView", glm::value_ptr(view));
        m_room_shader->SetMat4("uProj", glm::value_ptr(proj));
        m_room_shader->SetVec3("uColor", 0.15f, 0.15f, 0.15f);

        glBindVertexArray(m_ground_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        m_room_shader->Unbind();
    }

    // --------------------------------------------------------------------------------
    void Renderer::RenderEyes(
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& sphere_pos,
        const glm::quat& sphere_orientation,
        float sphere_radius,
        const BaldPatchConfig& bald_patches,
        const glm::vec3& mouse_ray_origin,
        const glm::vec3& mouse_ray_dir,
        const glm::vec3& camera_forward,
        bool  frozen)
    {
        float eye_radius = sphere_radius * 0.18f;
        float pupil_radius = eye_radius * 0.6f;

        glm::vec3 eye_dirs[2] = {
            glm::normalize(glm::vec3(
                bald_patches.eye_left_dir.x,
                bald_patches.eye_left_dir.y,
                bald_patches.eye_left_dir.z)),
            glm::normalize(glm::vec3(
                bald_patches.eye_right_dir.x,
                bald_patches.eye_right_dir.y,
                bald_patches.eye_right_dir.z))
        };

        for (int i = 0; i < 2; i++)
        {
            glm::vec3 eye_local = eye_dirs[i] * sphere_radius * 0.85f;
            glm::vec3 eye_world = sphere_pos
                + QuatRotate(sphere_orientation, eye_local);

            // white sphere
            glm::mat4 eye_scale = glm::scale(
                glm::mat4(1.0f),
                glm::vec3(eye_radius / sphere_radius));
            glm::mat4 eye_model = glm::translate(glm::mat4(1.0f), eye_world)
                * eye_scale;

            m_sphere_shader->Bind();
            m_sphere_shader->SetMat4("uModel", glm::value_ptr(eye_model));
            m_sphere_shader->SetMat4("uView", glm::value_ptr(view));
            m_sphere_shader->SetMat4("uProj", glm::value_ptr(proj));
            m_sphere_shader->SetVec3("uColor", 0.95f, 0.95f, 0.95f);

            glBindVertexArray(m_sphere_vao);
            glDrawElements(GL_TRIANGLES, m_sphere_index_count, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // per-eye mouse world intersection
            glm::vec3 eye_forward = glm::normalize(eye_world - sphere_pos);
            glm::vec3 eye_mouse_world = eye_world; // fallback
            RayUtils::RayCameraPlaneIntersect(
                { mouse_ray_origin, mouse_ray_dir },
                eye_world,        // <- plane at eye position
                camera_forward,
                eye_mouse_world);

            glm::vec3 projected(0.0f);

            if (!frozen)
            {
                glm::vec3 to_mouse = eye_mouse_world - eye_world;
                float     max_offset = eye_radius * 0.25f;

                projected = to_mouse - glm::dot(to_mouse, eye_forward) * eye_forward;

                float proj_len = glm::length(projected);
                if (proj_len > max_offset)
                    projected = projected / proj_len * max_offset;
            }

            glm::vec3 pupil_world = eye_world
                + eye_forward * eye_radius * 0.5f
                + projected;

            glm::mat4 pupil_scale = glm::scale(
                glm::mat4(1.0f),
                glm::vec3(pupil_radius / sphere_radius));
            glm::mat4 pupil_model = glm::translate(glm::mat4(1.0f), pupil_world)
                * pupil_scale;

            m_sphere_shader->SetMat4("uModel", glm::value_ptr(pupil_model));
            m_sphere_shader->SetVec3("uColor", 0.1f, 0.1f, 0.1f);

            glBindVertexArray(m_sphere_vao);
            glDrawElements(GL_TRIANGLES, m_sphere_index_count, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            m_sphere_shader->Unbind();
        }
    }

    // --------------------------------------------------------------------------------
    void Renderer::Shutdown()
    {
        if (m_cuda_vbo_resource)
            cudaGraphicsUnregisterResource(
                reinterpret_cast<cudaGraphicsResource*>(m_cuda_vbo_resource));

        glDeleteVertexArrays(1, &m_hair_vao);
        glDeleteBuffers(1, &m_hair_vbo);
        glDeleteBuffers(1, &m_hair_ebo);

        glDeleteVertexArrays(1, &m_sphere_vao);
        glDeleteBuffers(1, &m_sphere_vbo);
        glDeleteBuffers(1, &m_sphere_ebo);

        glDeleteVertexArrays(1, &m_room_vao);
        glDeleteBuffers(1, &m_room_vbo);

        glDeleteVertexArrays(1, &m_ground_vao);
        glDeleteBuffers(1, &m_ground_vbo);

        delete m_hair_shader;
        delete m_sphere_shader;
        delete m_room_shader;

        m_initialized = false;
        std::cout << "[INFO] Renderer shutdown." << std::endl;
    }

    // --------------------------------------------------------------------------------
    glm::vec3 Renderer::GetEyeWorldPos(
        int                    eye_index,
        const glm::vec3& sphere_pos,
        const glm::quat& sphere_orientation,
        float                  sphere_radius,
        const BaldPatchConfig& bald_patches) const
    {
        glm::vec3 eye_dirs[2] = {
            glm::normalize(glm::vec3(
                bald_patches.eye_left_dir.x,
                bald_patches.eye_left_dir.y,
                bald_patches.eye_left_dir.z)),
            glm::normalize(glm::vec3(
                bald_patches.eye_right_dir.x,
                bald_patches.eye_right_dir.y,
                bald_patches.eye_right_dir.z))
        };

        glm::vec3 eye_local = eye_dirs[eye_index] * sphere_radius * 0.85f;
        return sphere_pos + QuatRotate(sphere_orientation, eye_local);
    }

    // --------------------------------------------------------------------------------
    glm::vec3 Renderer::GetEyeOutwardNormal(
        int                    eye_index,
        const glm::vec3& sphere_pos,
        const glm::quat& sphere_orientation,
        float                  sphere_radius,
        const BaldPatchConfig& bald_patches) const
    {
        glm::vec3 eye_world = GetEyeWorldPos(
            eye_index, sphere_pos, sphere_orientation,
            sphere_radius, bald_patches);

        return glm::normalize(eye_world - sphere_pos);
    }

} // namespace Toop