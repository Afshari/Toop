#pragma once
#ifndef TOOP_HEADLESS
#ifdef TOOP_DEBUG
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace Toop {

    struct DebugVertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    struct DebugFillVertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        float     alpha;
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

        void AddWireCube(const glm::vec3& center, const glm::quat& orientation,
            const glm::vec3& half_extents, const glm::vec3& color);

        // persistent lines - survive across frames until cleared
        void AddPersistentLine(const glm::vec3& start, const glm::vec3& end,
            const glm::vec3& color);
        void ClearPersistent();
        int  GetPersistentCount() const { return (int)m_persistent_vertices.size() / 2; }

        // filled transparent quads - per-frame and persistent (snapshot) variants
        void AddFilledPlane(const glm::vec3& center, const glm::vec3& normal,
            float size, const glm::vec3& color, float alpha);
        void AddPersistentFilledPlane(const glm::vec3& center, const glm::vec3& normal,
            float size, const glm::vec3& color, float alpha);
        void ClearPersistentFilled();

        void AddPersistentWireCube(const glm::vec3& center, const glm::quat& orientation,
            const glm::vec3& half_extents, const glm::vec3& color);

        // render all primitives and clear for next frame
        void Render(const glm::mat4& view, const glm::mat4& proj);

        bool IsInitialized() const { return m_initialized; }

    private:
        void BuildShader();
        void BuildFillShader();

        GLuint m_vao = 0;
        GLuint m_vbo = 0;
        GLuint m_shader = 0;
        bool   m_initialized = false;

        GLuint m_fill_vao = 0;
        GLuint m_fill_vbo = 0;
        GLuint m_fill_shader = 0;

        std::vector<DebugVertex> m_vertices;
        std::vector<DebugVertex> m_persistent_vertices;

        std::vector<DebugFillVertex> m_fill_vertices;
        std::vector<DebugFillVertex> m_persistent_fill_vertices;
    };

} // namespace Toop
#endif // TOOP_DEBUG
#endif // TOOP_HEADLESS
