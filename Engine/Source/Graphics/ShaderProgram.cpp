
#include "ShaderProgram.h"
#include <glm/gtc/type_ptr.hpp>

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    ShaderProgram::ShaderProgram(const GLuint &_shaderProgramID) : m_shaderProgramID(_shaderProgramID) {}

    ShaderProgram::~ShaderProgram()
    {
        glDeleteProgram(m_shaderProgramID);
    }

    void ShaderProgram::Bind() const
    {
        glUseProgram(m_shaderProgramID);
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    GLint ShaderProgram::GetUniformLocation(const std::string& _name)
    {
        // Search cache, return uniform location if found
        auto it = m_uniformLocationCache.find(_name);
        if (it != m_uniformLocationCache.end())
        {
            return it->second;
        }

        // If name is not found, create an entry
        const GLint location = glGetUniformLocation(m_shaderProgramID, _name.c_str());
        m_uniformLocationCache[_name] = location;
        return location;
    }

    /**
     * @brief Sets the uniform to desired value in the current shader program
     * @param _name Name of the uniform you want to write to
     * @param _v0 float to write to uniform
     */
    void ShaderProgram::SetUniform(const std::string& _name, const float _v0)
    {
        auto location = GetUniformLocation(_name);
        glUniform1f(location, _v0);
    }

    /**
     * @brief Sets the uniform to desired value in the current shader program
     * @param _name Name of the uniform you want to write to
     * @param _v0 float 1 to write to uniform
     * @param _v1 float 2 to write to uniform
     */
    void ShaderProgram::SetUniform(const std::string& _name, float _v0, float _v1)
    {
        auto location = GetUniformLocation(_name);
        glUniform2f(location, _v0, _v1);
    }

    /**
     * @brief Sets the uniform to desired matrix in the current shader program
     * @param _name Name of the uniform to write to
     * @param _mat Transform matrix to apply
     */
    void ShaderProgram::SetUniform(const std::string& _name, const mat4& _mat)
    {
        auto location = GetUniformLocation(_name);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(_mat));
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------
}
