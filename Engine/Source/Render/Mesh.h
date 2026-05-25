
#pragma once
#include <GL/glew.h>
#include "Graphics/VertexLayout.h"

namespace RR
{
    /**
     * Hold geometry, therefore EBO, PBO, VAO.
     */
    class Mesh
    {
    public:
        Mesh(const VertexLayout& _layout, const std::vector<float>& _vertices, const std::vector<uint32_t>& _indices);
        Mesh(const VertexLayout& _layout, const std::vector<float>& _vertices);

        ~Mesh();

        // Prevent resource duplication
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        void Bind();
        void Draw();

    private:
        VertexLayout m_vertexLayout;

        GLuint m_VBO = 0;
        GLuint m_EBO = 0;
        GLuint m_VAO = 0;

        size_t m_vertexCount = 0;
        size_t m_indexCount  = 0;
    };
}
