
#pragma once
#include <GL/glew.h>
#include <memory>
#include <string>
#include <vector>

namespace RR
{
    class ShaderProgram;
    class Material;
    class Mesh;

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
        GraphicsAPI(GraphicsAPI&&) noexcept = delete;
        GraphicsAPI& operator=(GraphicsAPI&&) noexcept = delete;

        // METHODS -----------------------------------------------------------------------------------------------------

        bool Init();

        std::shared_ptr<ShaderProgram> CreateShaderProgram(const std::string& _vertexShaderSource,
            const std::string& _fragmentShaderSource);

        const std::shared_ptr<ShaderProgram>& GetDefaultShaderProgram();

        GLuint CreateVertexBufferObject(const std::vector<float>& _vertices);
        GLuint CreateElementBufferObject(const std::vector<uint32_t>& _indices);

        void BindShaderProgram(ShaderProgram* _shaderProgram);
        void BindMaterial(Material* _material);
        void BindMesh(Mesh* _mesh);
        void UnBindMesh(Mesh* _mesh);
        void UnbindVertexArray();          
        void DrawMesh(Mesh* _mesh);
        void ClearBuffers();

        static void SetBackfaceCulling(bool _enabled);
        static void SetDepthTest(bool _enabled);
        static void SetBlend(bool _enabled);
        static void SetClearColor(const vec4& _color = {1.0f, 1.0f, 1.0, 1.0f});

    private:
        std::shared_ptr<ShaderProgram> m_defaultShaderProgram;
    };
}






