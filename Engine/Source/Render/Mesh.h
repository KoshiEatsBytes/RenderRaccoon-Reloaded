
#pragma once
#include <memory>
#include <GL/glew.h>
#include "Graphics/VertexLayout.h"
#include "Helpers/Types.h"

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

        // specify move contrustors
        Mesh(Mesh&& _out) noexcept;
        Mesh& operator=(Mesh&& _out) noexcept = delete;

        void Bind();
        void UnBind();
        void Draw();

        sizeT GetIndexCount() const;

        static std::shared_ptr<Mesh> CreateBox(const vec3& _extents = vec3(1.0f));
        static std::shared_ptr<Mesh> CreateSphere(float _radius, float _sectors, float _stacks);

    private:
        VertexLayout m_vertexLayout;

        GLuint m_VBO = 0;
        GLuint m_EBO = 0;
        GLuint m_VAO = 0;

        size_t m_vertexCount = 0;
        size_t m_indexCount  = 0;

        static constexpr float PI = 3.14159265358979323846f;
    };
}
