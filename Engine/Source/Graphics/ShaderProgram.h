
#pragma once
#include <string>
#include <unordered_map>
#include <GL/glew.h>
#include <../Helpers/Types.h>

namespace RR
{
    class Texture;

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

        void Bind();

        GLint GetUniformLocation(const std::string& _name);
        void SetUniform(const std::string& _name, float _v0);
        void SetUniform(const std::string& _name, float _v0, float _v1);
        void SetUniform(const std::string& _name, const vec3& _vec);
        void SetUniform(const std::string& _name, const mat4& _mat);

        void SetTexture(const std::string& _name, Texture* _texture);

    private:
        // Use a cache to avoid redundant openGL call
        std::unordered_map<std::string, GLint> m_uniformLocationCache;

        GLuint m_shaderProgramID = 0;
        int m_currentTextureUnit = 0;
    };
}

