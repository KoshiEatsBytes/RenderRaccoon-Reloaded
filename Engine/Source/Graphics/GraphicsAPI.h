
#pragma once
#include <GL/glew.h>
#include <memory>
#include <string>
#include <vector>

namespace RR
{
    class ShaderProgram;
    class Material;

    /**
     * @details Centralized interface for rendering operations
     */
    class GraphicsAPI
    {
        GraphicsAPI();
        ~GraphicsAPI();

    public:
        // Only engine can construct this class
        friend class Engine;

        // Delete copy
        GraphicsAPI(const GraphicsAPI&) = delete;
        GraphicsAPI& operator=(const GraphicsAPI&) = delete;

        // Delete move
        GraphicsAPI(GraphicsAPI&&) = delete;
        GraphicsAPI& operator=(GraphicsAPI&&) = delete;

        // METHODS -----------------------------------------------------------------------------------------------------

        std::shared_ptr<ShaderProgram> CreateShaderProgram(const std::string& _vertexPath,
            const std::string& _fragmentPath);

        GLuint CreateVertexBufferObject(const std::vector<float>& _vertices);
        GLuint CreateElementBufferObject(const std::vector<uint32_t>& _indices);

        void BindShaderProgram(ShaderProgram* _shaderProgram);

        void BindMaterial(Material* _material);
    };
}

