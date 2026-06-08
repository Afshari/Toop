#include "cpu/ShaderProgram.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace Toop {

    // --------------------------------------------------------------------------------
    ShaderProgram::ShaderProgram(
        const std::string& vert_path,
        const std::string& frag_path)
    {
        GLuint vert = CompileShader(vert_path, GL_VERTEX_SHADER);
        GLuint frag = CompileShader(frag_path, GL_FRAGMENT_SHADER);

        m_program = glCreateProgram();
        glAttachShader(m_program, vert);
        glAttachShader(m_program, frag);
        glLinkProgram(m_program);

        // check link errors
        GLint success = 0;
        glGetProgramiv(m_program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetProgramInfoLog(m_program, 512, nullptr, log);
            glDeleteProgram(m_program);
            m_program = 0;
            throw std::runtime_error(
                std::string("[ERROR] ShaderProgram link failed: ") + log);
        }

        // shaders linked - no longer needed
        glDeleteShader(vert);
        glDeleteShader(frag);

        std::cout << "[INFO] ShaderProgram linked: "
            << vert_path << " + " << frag_path << std::endl;
    }

    // --------------------------------------------------------------------------------
    ShaderProgram::~ShaderProgram()
    {
        if (m_program != 0)
            glDeleteProgram(m_program);
    }

    // --------------------------------------------------------------------------------
    ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
        : m_program(other.m_program)
        , m_uniform_cache(std::move(other.m_uniform_cache))
    {
        other.m_program = 0;
    }

    // --------------------------------------------------------------------------------
    ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
    {
        if (this != &other)
        {
            if (m_program != 0)
                glDeleteProgram(m_program);
            m_program = other.m_program;
            m_uniform_cache = std::move(other.m_uniform_cache);
            other.m_program = 0;
        }
        return *this;
    }

    // --------------------------------------------------------------------------------
    void ShaderProgram::Bind()   const { glUseProgram(m_program); }
    void ShaderProgram::Unbind() const { glUseProgram(0); }

    // --------------------------------------------------------------------------------
    GLuint ShaderProgram::CompileShader(
        const std::string& path,
        GLenum             type) const
    {
        std::string source = ReadFile(path);
        const char* src = source.c_str();

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            glDeleteShader(shader);
            throw std::runtime_error(
                std::string("[ERROR] Shader compile failed: ") + path + "\n" + log);
        }

        return shader;
    }

    // --------------------------------------------------------------------------------
    std::string ShaderProgram::ReadFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error(
                "[ERROR] ShaderProgram cannot open file: " + path);

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    // --------------------------------------------------------------------------------
    GLint ShaderProgram::GetUniformLocation(const std::string& name) const
    {
        auto it = m_uniform_cache.find(name);
        if (it != m_uniform_cache.end())
            return it->second;

        GLint loc = glGetUniformLocation(m_program, name.c_str());
        if (loc == -1)
            std::cerr << "[WARN] Uniform not found: " << name << std::endl;

        m_uniform_cache[name] = loc;
        return loc;
    }

    // --------------------------------------------------------------------------------
    void ShaderProgram::SetInt(const std::string& name, int value) const
    {
        glUniform1i(GetUniformLocation(name), value);
    }

    void ShaderProgram::SetFloat(const std::string& name, float value) const
    {
        glUniform1f(GetUniformLocation(name), value);
    }

    void ShaderProgram::SetVec3(
        const std::string& name,
        float x, float y, float z) const
    {
        glUniform3f(GetUniformLocation(name), x, y, z);
    }

    void ShaderProgram::SetMat4(
        const std::string& name,
        const float* matrix) const
    {
        glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, matrix);
    }

} // namespace Toop