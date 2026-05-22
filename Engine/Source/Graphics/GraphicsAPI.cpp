
#include "GraphicsApi.h"
#include "ShaderProgram.h"
#include "Helpers/ShaderLoader.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    GraphicsAPI::GraphicsAPI() = default;

    GraphicsAPI::~GraphicsAPI() = default;

    std::shared_ptr<ShaderProgram> GraphicsAPI::CreateShaderProgram(const std::string &_vertexPath,
        const std::string &_fragmentPath)
    {
        // VERTEX SHADER -----------------------------------------------------------------------------------------------

        // Load vertex shader from file and compile it
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        std::string vertexShaderSource = ShaderLoader::LoadFromFile(_vertexPath);

        // check if valid path
        if (vertexShaderSource.empty())
        {
            Error("[VERTEX SHADER] Could not load file at: ", _vertexPath);
            glDeleteShader(vertexShader);
            return nullptr;
        }

        const char* vertexShaderSourceCStr = vertexShaderSource.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSourceCStr, nullptr);
        glCompileShader(vertexShader);

        // Check if vertex has compiled correctly
        GLint success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            Error("[VERTEX SHADER] Compilation Failed! File: ", _vertexPath);
            InfoLog(infoLog);

            // Clean-up
            glDeleteShader(vertexShader);
            return nullptr;
        }

        // FRAGMENT SHADER ---------------------------------------------------------------------------------------------

        // Load fragment shader from file and compile it
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        std::string fragmentShaderSource = ShaderLoader::LoadFromFile(_fragmentPath);

        // check if valid path
        if (fragmentShaderSource.empty())
        {
            Error("[FRAGMENT SHADER] Could not load file at: ", _fragmentPath);
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            return nullptr;
        }

        const char* fragmentShaderSourceCStr = fragmentShaderSource.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, nullptr);
        glCompileShader(fragmentShader);

        // Check if fragment has compiled correctly
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            Error("[FRAGMENT SHADER] Compilation Failed! File: ", _fragmentPath);
            InfoLog(infoLog);

            // cleanup
            glDeleteShader(fragmentShader);
            glDeleteShader(vertexShader);
            return nullptr;
        }

        // SHADER PROGRAM ----------------------------------------------------------------------------------------------

        // Link previous shaders to shader program
        GLuint shaderProgramID = glCreateProgram();
        glAttachShader(shaderProgramID, vertexShader);
        glAttachShader(shaderProgramID, fragmentShader);
        glLinkProgram(shaderProgramID);

        // check if link successful
        glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgramID, 512, nullptr, infoLog);
            Error("[SHADER LINKING] Shader Program Linking Failed! Program ID: ", shaderProgramID);
            InfoLog(infoLog);

            // cleanup
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glDeleteProgram(shaderProgramID);
            return nullptr;
        }

        // Delete shader after gpu upload
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // after success wrap in shaderProgramID
        return std::make_shared<ShaderProgram>(shaderProgramID);
    }

    void GraphicsAPI::BindShaderProgram(ShaderProgram* _shaderProgram)
    {
        _shaderProgram->Bind();
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

}

























