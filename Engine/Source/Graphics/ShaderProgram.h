
#pragma once
#include <string>
#include <unordered_map>
#include <GL/glew.h>

namespace RR
{
    /**
     * @brief Copy deleted as this ensure each shader is created and destroyed
     * exactly once, currently further restrictions are not in place
     */
    class ShaderProgram
    {
    public:
        explicit ShaderProgram(const GLuint& _shaderProgramID);
        ~ShaderProgram();

        // def and copy deleted
        ShaderProgram() = delete;
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;

        void Bind() const;

        GLint GetUniformLocation(const std::string& _name);
        void SetUniform(const std::string& _name, const float& _value);

    private:
        // Use a cache to avoid redundant openGL call
        std::unordered_map<std::string, GLint> m_uniformLocationCache;

        GLuint m_shaderProgramID = 0;
    };
}

