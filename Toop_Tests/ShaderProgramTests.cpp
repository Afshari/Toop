#include "pch.h"
#include "cpu/ShaderProgram.h"
#include "TestHelpers.h"
#include <filesystem>
#include <fstream>

namespace Toop {

    // --------------------------------------------------------------------------------
    // helper to write a temp file
    // --------------------------------------------------------------------------------
    static std::filesystem::path WriteTempFile(
        const std::string& filename,
        const std::string& content)
    {
        auto path = std::filesystem::temp_directory_path() / filename;
        std::ofstream f(path);
        f << content;
        return path;
    }

    // --------------------------------------------------------------------------------
    // ReadFile tests
    // --------------------------------------------------------------------------------

    TEST(ShaderProgramTest, ReadFileThrowsOnMissingFile)
    {
        EXPECT_THROW(
            ShaderProgram::ReadFile("nonexistent_shader.glsl"),
            std::runtime_error);
    }

    TEST(ShaderProgramTest, ReadFileReturnsCorrectContent)
    {
        auto path = WriteTempFile("test_shader.glsl", "void main() {}");
        std::string content = ShaderProgram::ReadFile(path.string());
        EXPECT_EQ(content, "void main() {}");
        std::filesystem::remove(path);
    }

    TEST(ShaderProgramTest, ReadFileReturnsEmptyStringForEmptyFile)
    {
        auto path = WriteTempFile("empty_shader.glsl", "");
        std::string content = ShaderProgram::ReadFile(path.string());
        EXPECT_EQ(content, "");
        std::filesystem::remove(path);
    }

    TEST(ShaderProgramTest, ReadFilePreservesNewlines)
    {
        std::string source = "#version 330 core\nvoid main() {\n    gl_Position = vec4(0.0);\n}";
        auto path = WriteTempFile("newline_shader.glsl", source);
        std::string content = ShaderProgram::ReadFile(path.string());
        EXPECT_EQ(content, source);
        std::filesystem::remove(path);
    }

    TEST(ShaderProgramTest, ReadFilePreservesWhitespace)
    {
        std::string source = "   uniform mat4 uMVP;   ";
        auto path = WriteTempFile("whitespace_shader.glsl", source);
        std::string content = ShaderProgram::ReadFile(path.string());
        EXPECT_EQ(content, source);
        std::filesystem::remove(path);
    }

} // namespace Toop