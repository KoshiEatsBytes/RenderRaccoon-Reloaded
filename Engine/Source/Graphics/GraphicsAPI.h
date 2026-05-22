
#pragma once
#include <GL/glew.h>
#include <memory>
#include <string>

namespace RR
{
    class ShaderProgram;

    /**
     * @details Centralized interface for rendering operations
     */
    class GraphicsAPI
    {
    public:
        GraphicsAPI();
        ~GraphicsAPI();

        std::shared_ptr<ShaderProgram> CreateShaderProgram(const std::string& _vertexPath,
            const std::string& _fragmentPath);

        void BindShaderProgram(ShaderProgram* _shaderProgram);
    };
}

