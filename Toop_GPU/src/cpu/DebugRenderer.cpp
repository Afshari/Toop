#include "cpu/DebugRenderer.h"

#ifdef TOOP_DEBUG

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

namespace Toop {

    // --------------------------------------------------------------------------------
    static const char* k_vert_src = R"(
        #version 430 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        uniform mat4 uView;
        uniform mat4 uProj;
        out vec3 vColor;
        void main()
        {
            vColor      = aColor;
            gl_Position = uProj * uView * vec4(aPos, 1.0);
        }
    )";

    static const char* k_frag_src = R"(
        #version 430 core
        in  vec3 vColor;
        out vec4 FragColor;
        void main()
        {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    static const char* k_fill_vert_src = R"(
        #version 430 core
        layout(location = 0) in vec3  aPos;
        layout(location = 1) in vec3  aColor;
        layout(location = 2) in float aAlpha;
        uniform mat4 uView;
        uniform mat4 uProj;
        out vec3  vColor;
        out float vAlpha;
        void main()
        {
            vColor      = aColor;
            vAlpha      = aAlpha;
            gl_Position = uProj * uView * vec4(aPos, 1.0);
        }
    )";

    static const char* k_fill_frag_src = R"(
        #version 430 core
        in  vec3  vColor;
        in  float vAlpha;
        out vec4  FragColor;
        void main()
        {
            FragColor = vec4(vColor, vAlpha);
        }
    )";

    // --------------------------------------------------------------------------------
    void DebugRenderer::Init()
    {
        BuildShader();

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        // pos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(DebugVertex), (void*)offsetof(DebugVertex, pos));
        glEnableVertexAttribArray(0);

        // color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(DebugVertex), (void*)offsetof(DebugVertex, color));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        m_vertices.reserve(1024);
        m_persistent_vertices.reserve(1024);

        // ----
        BuildFillShader();

        glGenVertexArrays(1, &m_fill_vao);
        glGenBuffers(1, &m_fill_vbo);

        glBindVertexArray(m_fill_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_fill_vbo);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            sizeof(DebugFillVertex), (void*)offsetof(DebugFillVertex, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
            sizeof(DebugFillVertex), (void*)offsetof(DebugFillVertex, color));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
            sizeof(DebugFillVertex), (void*)offsetof(DebugFillVertex, alpha));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        m_fill_vertices.reserve(256);
        m_persistent_fill_vertices.reserve(256);

        m_initialized = true;
        std::cout << "[INFO] DebugRenderer initialized." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::Shutdown()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteProgram(m_shader);

        glDeleteVertexArrays(1, &m_fill_vao);
        glDeleteBuffers(1, &m_fill_vbo);
        glDeleteProgram(m_fill_shader);

        m_initialized = false;
        std::cout << "[INFO] DebugRenderer shutdown." << std::endl;
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::BuildShader()
    {
        auto compile = [](const char* src, GLenum type) -> GLuint
            {
                GLuint s = glCreateShader(type);
                glShaderSource(s, 1, &src, nullptr);
                glCompileShader(s);
                GLint ok = 0;
                glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
                if (!ok)
                {
                    char log[512];
                    glGetShaderInfoLog(s, 512, nullptr, log);
                    throw std::runtime_error(
                        std::string("[ERROR] DebugRenderer shader: ") + log);
                }
                return s;
            };

        GLuint vert = compile(k_vert_src, GL_VERTEX_SHADER);
        GLuint frag = compile(k_frag_src, GL_FRAGMENT_SHADER);

        m_shader = glCreateProgram();
        glAttachShader(m_shader, vert);
        glAttachShader(m_shader, frag);
        glLinkProgram(m_shader);

        GLint ok = 0;
        glGetProgramiv(m_shader, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetProgramInfoLog(m_shader, 512, nullptr, log);
            throw std::runtime_error(
                std::string("[ERROR] DebugRenderer link: ") + log);
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::BuildFillShader()
    {
        auto compile = [](const char* src, GLenum type) -> GLuint
            {
                GLuint s = glCreateShader(type);
                glShaderSource(s, 1, &src, nullptr);
                glCompileShader(s);
                GLint ok = 0;
                glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
                if (!ok)
                {
                    char log[512];
                    glGetShaderInfoLog(s, 512, nullptr, log);
                    throw std::runtime_error(
                        std::string("[ERROR] DebugRenderer fill shader: ") + log);
                }
                return s;
            };

        GLuint vert = compile(k_fill_vert_src, GL_VERTEX_SHADER);
        GLuint frag = compile(k_fill_frag_src, GL_FRAGMENT_SHADER);

        m_fill_shader = glCreateProgram();
        glAttachShader(m_fill_shader, vert);
        glAttachShader(m_fill_shader, frag);
        glLinkProgram(m_fill_shader);

        GLint ok = 0;
        glGetProgramiv(m_fill_shader, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetProgramInfoLog(m_fill_shader, 512, nullptr, log);
            throw std::runtime_error(
                std::string("[ERROR] DebugRenderer fill link: ") + log);
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddLine(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color)
    {
        m_vertices.push_back({ start, color });
        m_vertices.push_back({ end,   color });
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddRay(
        const glm::vec3& origin,
        const glm::vec3& dir,
        float            length,
        const glm::vec3& color)
    {
        AddLine(origin, origin + glm::normalize(dir) * length, color);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddPlane(
        const glm::vec3& center,
        const glm::vec3& normal,
        float            size,
        const glm::vec3& color)
    {
        // build two tangent vectors perpendicular to normal
        glm::vec3 up = fabsf(normal.y) < 0.9f
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(normal, up));
        glm::vec3 bitan = glm::normalize(glm::cross(normal, tangent));

        float h = size * 0.5f;

        glm::vec3 p0 = center + (-tangent - bitan) * h;
        glm::vec3 p1 = center + (tangent - bitan) * h;
        glm::vec3 p2 = center + (tangent + bitan) * h;
        glm::vec3 p3 = center + (-tangent + bitan) * h;

        // 4 edges of the quad
        AddLine(p0, p1, color);
        AddLine(p1, p2, color);
        AddLine(p2, p3, color);
        AddLine(p3, p0, color);

        // cross lines to make it more visible
        AddLine(p0, p2, color);
        AddLine(p1, p3, color);

        // normal indicator
        AddLine(center, center + normal * h * 0.5f, color);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddArrow(
        const glm::vec3& origin,
        const glm::vec3& dir,
        float            scale,
        const glm::vec3& color)
    {
        if (glm::length(dir) < 1e-6f) return;

        glm::vec3 end = origin + dir * scale;
        AddLine(origin, end, color);

        // small cross at tip
        glm::vec3 up = fabsf(dir.y) < 0.9f
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(dir, up));
        glm::vec3 up2 = glm::normalize(glm::cross(right, dir));

        float tip = scale * 0.15f;
        AddLine(end - right * tip, end + right * tip, color);
        AddLine(end - up2 * tip, end + up2 * tip, color);
    }

    // --------------------------------------------------------------------------------
    static void BuildWireCubeLines(
        const glm::vec3& center,
        const glm::quat& orientation,
        const glm::vec3& half_extents,
        const glm::vec3& color,
        std::vector<DebugVertex>& out)
    {
        glm::vec3 local[8] = {
            { -half_extents.x, -half_extents.y, -half_extents.z },
            {  half_extents.x, -half_extents.y, -half_extents.z },
            {  half_extents.x,  half_extents.y, -half_extents.z },
            { -half_extents.x,  half_extents.y, -half_extents.z },
            { -half_extents.x, -half_extents.y,  half_extents.z },
            {  half_extents.x, -half_extents.y,  half_extents.z },
            {  half_extents.x,  half_extents.y,  half_extents.z },
            { -half_extents.x,  half_extents.y,  half_extents.z },
        };

        glm::vec3 world[8];
        for (int i = 0; i < 8; i++)
            world[i] = center + (orientation * local[i]);

        int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0},
            {4,5}, {5,6}, {6,7}, {7,4},
            {0,4}, {1,5}, {2,6}, {3,7}
        };

        for (auto& e : edges)
        {
            out.push_back({ world[e[0]], color });
            out.push_back({ world[e[1]], color });
        }
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddWireCube(
        const glm::vec3& center,
        const glm::quat& orientation,
        const glm::vec3& half_extents,
        const glm::vec3& color)
    {
        BuildWireCubeLines(center, orientation, half_extents, color, m_vertices);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddPersistentWireCube(
        const glm::vec3& center,
        const glm::quat& orientation,
        const glm::vec3& half_extents,
        const glm::vec3& color)
    {
        BuildWireCubeLines(center, orientation, half_extents, color, m_persistent_vertices);
    }

    // --------------------------------------------------------------------------------
    static void BuildQuadTris(
        const glm::vec3& center,
        const glm::vec3& normal,
        float             size,
        const glm::vec3& color,
        float             alpha,
        std::vector<DebugFillVertex>& out)
    {
        glm::vec3 up = fabsf(normal.y) < 0.9f
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(normal, up));
        glm::vec3 bitan = glm::normalize(glm::cross(normal, tangent));

        float h = size * 0.5f;

        glm::vec3 p0 = center + (-tangent - bitan) * h;
        glm::vec3 p1 = center + (tangent - bitan) * h;
        glm::vec3 p2 = center + (tangent + bitan) * h;
        glm::vec3 p3 = center + (-tangent + bitan) * h;

        // two triangles: p0-p1-p2, p0-p2-p3
        out.push_back({ p0, color, alpha });
        out.push_back({ p1, color, alpha });
        out.push_back({ p2, color, alpha });

        out.push_back({ p0, color, alpha });
        out.push_back({ p2, color, alpha });
        out.push_back({ p3, color, alpha });
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddFilledPlane(
        const glm::vec3& center,
        const glm::vec3& normal,
        float             size,
        const glm::vec3& color,
        float             alpha)
    {
        BuildQuadTris(center, normal, size, color, alpha, m_fill_vertices);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddPersistentFilledPlane(
        const glm::vec3& center,
        const glm::vec3& normal,
        float             size,
        const glm::vec3& color,
        float             alpha)
    {
        BuildQuadTris(center, normal, size, color, alpha, m_persistent_fill_vertices);
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::ClearPersistentFilled()
    {
        m_persistent_fill_vertices.clear();
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::Render(
        const glm::mat4& view,
        const glm::mat4& proj)
    {
        if (m_vertices.empty() && m_persistent_vertices.empty()
            && m_fill_vertices.empty() && m_persistent_fill_vertices.empty())
            return;

        glUseProgram(m_shader);
        glUniformMatrix4fv(
            glGetUniformLocation(m_shader, "uView"),
            1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(
            glGetUniformLocation(m_shader, "uProj"),
            1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        if (!m_vertices.empty())
        {
            glBufferData(GL_ARRAY_BUFFER,
                m_vertices.size() * sizeof(DebugVertex),
                m_vertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINES, 0, (GLsizei)m_vertices.size());
            m_vertices.clear();
        }

        if (!m_persistent_vertices.empty())
        {
            glBufferData(GL_ARRAY_BUFFER,
                m_persistent_vertices.size() * sizeof(DebugVertex),
                m_persistent_vertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINES, 0, (GLsizei)m_persistent_vertices.size());
        }

        glBindVertexArray(0);
        glUseProgram(0);

        // filled transparent quads
        if (!m_fill_vertices.empty() || !m_persistent_fill_vertices.empty())
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            glUseProgram(m_fill_shader);
            glUniformMatrix4fv(
                glGetUniformLocation(m_fill_shader, "uView"),
                1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(
                glGetUniformLocation(m_fill_shader, "uProj"),
                1, GL_FALSE, glm::value_ptr(proj));

            glBindVertexArray(m_fill_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_fill_vbo);

            if (!m_fill_vertices.empty())
            {
                glBufferData(GL_ARRAY_BUFFER,
                    m_fill_vertices.size() * sizeof(DebugFillVertex),
                    m_fill_vertices.data(), GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_fill_vertices.size());
                m_fill_vertices.clear();
            }

            if (!m_persistent_fill_vertices.empty())
            {
                glBufferData(GL_ARRAY_BUFFER,
                    m_persistent_fill_vertices.size() * sizeof(DebugFillVertex),
                    m_persistent_fill_vertices.data(), GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_persistent_fill_vertices.size());
            }

            glBindVertexArray(0);
            glUseProgram(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::AddPersistentLine(
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color)
    {
        m_persistent_vertices.push_back({ start, color });
        m_persistent_vertices.push_back({ end,   color });
    }

    // --------------------------------------------------------------------------------
    void DebugRenderer::ClearPersistent()
    {
        m_persistent_vertices.clear();
    }

} // namespace Toop

#endif // TOOP_DEBUG