#pragma once
#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace Toop {

    struct DebugVertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    class DebugRenderer
    {
    public:
        void Init();
        void Shutdown();

        // add primitives each frame
        void AddLine(const glm::vec3& start, const glm::vec3& end,
            const glm::vec3& color);
        void AddRay(const glm::vec3& origin, const glm::vec3& dir,
            float length, const glm::vec3& color);
        void AddPlane(const glm::vec3& center, const glm::vec3& normal,
            float size, const glm::vec3& color);
        void AddArrow(const glm::vec3& origin, const glm::vec3& dir,
            float scale, const glm::vec3& color);

        // persistent lines - survive across frames until cleared
        void AddPersistentLine(const glm::vec3& start, const glm::vec3& end,
            const glm::vec3& color);
        void ClearPersistent();
        int  GetPersistentCount() const { return (int)m_persistent_vertices.size() / 2; }

        // render all primitives and clear for next frame
        void Render(const glm::mat4& view, const glm::mat4& proj);

        bool IsInitialized() const { return m_initialized; }

    private:
        void BuildShader();

        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_shader = 0;
        bool   m_initialized = false;

        std::vector<DebugVertex> m_vertices;
        std::vector<DebugVertex> m_persistent_vertices;
    };

} // namespace Toop
#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS
