#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>

namespace Toop {

    class ShaderProgram
    {
    public:
        // RAII - loads and compiles shaders immediately
        ShaderProgram(
            const std::string& vert_path,
            const std::string& frag_path);

        ~ShaderProgram();

        // non-copyable, movable
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&& other)            noexcept;
        ShaderProgram& operator=(ShaderProgram&& other) noexcept;

        void Bind()   const;
        void Unbind() const;

        bool IsValid() const { return m_program != 0; }

        // uniform setters
        void SetInt(const std::string& name, int value)          const;
        void SetFloat(const std::string& name, float value)        const;
        void SetVec3(const std::string& name, float x, float y, float z) const;
        void SetMat4(const std::string& name, const float* matrix) const;

        static std::string ReadFile(const std::string& path);

    private:
        GLuint CompileShader(const std::string& path, GLenum type) const;
        GLint  GetUniformLocation(const std::string& name)         const;

        GLuint m_program = 0;

        // cache uniform locations - avoids repeated glGetUniformLocation calls
        mutable std::unordered_map<std::string, GLint> m_uniform_cache;
    };

} // namespace Toop