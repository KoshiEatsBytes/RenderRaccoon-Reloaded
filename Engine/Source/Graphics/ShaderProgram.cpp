
#include "ShaderProgram.h"

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
     * @brief This is an example wrapper or a set uniform func
     * @param _name Name of the uniform you want to write to
     * @param _value Value which is being written to the uniform
     */
    void ShaderProgram::SetUniform(const std::string& _name, const float& _value)
    {
        auto location = GetUniformLocation(_name);

        glUniform1f(location, _value);
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

}
