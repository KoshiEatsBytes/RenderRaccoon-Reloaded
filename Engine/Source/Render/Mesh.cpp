
#include <string>

#include "Mesh.h"
#include "Graphics/GraphicsAPI.h"
#include "Engine.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Mesh::Mesh(const VertexLayout& _layout, const std::vector<float>& _vertices, const std::vector<uint32_t>& _indices)
    {
        m_vertexLayout = _layout;

        auto& graphicsAPI = Engine::GetInstance().GetGraphicsAPI();

        m_VBO = graphicsAPI.CreateVertexBufferObject(_vertices);
        m_EBO = graphicsAPI.CreateElementBufferObject(_indices);

        // Generated and bind VAO
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        // Configure vertex attributes from layout
        for (auto& element : m_vertexLayout.elements)
        {
            glVertexAttribPointer(element.index, element.size, element.type, GL_FALSE,
                m_vertexLayout.stride, (void*)(uintptr_t)element.offset);

            glEnableVertexAttribArray(element.index);
        }

        // Bind indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);

        // All binding done - Cleanup
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Calculate vertex and index count
        m_vertexCount = (_vertices.size() * sizeof(float)) / m_vertexLayout.stride;
        m_indexCount = _indices.size();
    }

    Mesh::Mesh(const VertexLayout& _layout, const std::vector<float>& _vertices)
    {
        m_vertexLayout = _layout;

        auto& graphicsAPI = Engine::GetInstance().GetGraphicsAPI();

        m_VBO = graphicsAPI.CreateVertexBufferObject(_vertices);

        // Generated and bind VAO
        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        // Configure vertex attributes from layout
        for (auto& element : m_vertexLayout.elements)
        {
            glVertexAttribPointer(element.index, element.size, element.type, GL_FALSE,
                m_vertexLayout.stride, (void*)(uintptr_t)element.offset);

            glEnableVertexAttribArray(element.index);
        }

        // All binding done - Cleanup
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Calculate vertex count
        m_vertexCount = (_vertices.size() * sizeof(float)) / m_vertexLayout.stride;
    }

    Mesh::~Mesh()
    = default;

    /**
     * Bind own VAO calling bind vertex array
     */
    void Mesh::Bind()
    {
        glBindVertexArray(m_VAO);
    }

    void Mesh::UnBind()
    {
        glBindVertexArray(0);
    }

    void Mesh::Draw()
    {
        // Are any indices used?
        if (m_indexCount > 0)
        {
            // Use index drawing
            glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
        }
    }

    std::shared_ptr<Mesh> Mesh::CreateBox(const vec3& _extents)
    {
        const glm::vec3 half = _extents * 0.5f;
        std::vector<float> vertices =
        {
            // Front face
            half.x, half.y, half.z, 1.0f, 0.0f, 0.0f, _extents.x, _extents.y, 0.0f, 0.0f, 1.0f,
            -half.x, half.y, half.z, 0.0f, 1.0f, 0.0f, 0.0f, _extents.y, 0.0f, 0.0f, 1.0f,
            -half.x, -half.y, half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            half.x, -half.y, half.z, 1.0f, 1.0f, 0.0f, _extents.x, 0.0f, 0.0f, 0.0f, 1.0f,

            // Top face
            half.x, half.y, -half.z, 1.0f, 0.0f, 0.0f, _extents.x, _extents.z, 0.0f, 1.0f, 0.0f,
            -half.x, half.y, -half.z, 0.0f, 1.0f, 0.0f, 0.0f, _extents.z, 0.0f, 1.0f, 0.0f,
            -half.x, half.y, half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            half.x, half.y, half.z, 1.0f, 1.0f, 0.0f, _extents.x, 0.0f, 0.0f, 1.0f, 0.0f,

            // Right face
            half.x, half.y, -half.z, 1.0f, 0.0f, 0.0f, _extents.z, _extents.y, 1.0f, 0.0f, 0.0f,
            half.x, half.y, half.z, 0.0f, 1.0f, 0.0f, 0.0f, _extents.y, 1.0f, 0.0f, 0.0f,
            half.x, -half.y, half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            half.x, -half.y, -half.z, 1.0f, 1.0f, 0.0f, _extents.z, 0.0f, 1.0f, 0.0f, 0.0f,

            // Left face
            -half.x, half.y, half.z, 1.0f, 0.0f, 0.0f, _extents.z, _extents.y, -1.0f, 0.0f, 0.0f,
            -half.x, half.y, -half.z, 0.0f, 1.0f, 0.0f, 0.0f, _extents.y, -1.0f, 0.0f, 0.0f,
            -half.x, -half.y, -half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            -half.x, -half.y, half.z, 1.0f, 1.0f, 0.0f, _extents.z, 0.0f, -1.0f, 0.0f, 0.0f,

            // Bottom face
            half.x, -half.y, half.z, 1.0f, 0.0f, 0.0f, _extents.x, _extents.z, 0.0f, -1.0f, 0.0f,
            -half.x, -half.y, half.z, 0.0f, 1.0f, 0.0f, 0.0f, _extents.z, 0.0f, -1.0f, 0.0f,
            -half.x, -half.y, -half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
            half.x, -half.y, -half.z, 1.0f, 1.0f, 0.0f, _extents.x, 0.0f, 0.0f, -1.0f, 0.0f,

            // Back face
            -half.x, half.y, -half.z, 1.0f, 0.0f, 0.0f, _extents.x, _extents.y, 0.0f, 0.0f, -1.0f,
            half.x, half.y, -half.z, 0.0f, 1.0f, 0.0f, 0.0f, _extents.y, 0.0f, 0.0f, -1.0f,
            half.x, -half.y, -half.z, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
            -half.x, -half.y, -half.z, 1.0f, 1.0f, 0.0f, _extents.x, 0.0f, 0.0f, 0.0f, -1.0f
        };

        std::vector<unsigned int> indices =
        {
            // front face
            0, 1, 2,
            0, 2, 3,
            // top face
            4, 5, 6,
            4, 6, 7,
            // right face
            8, 9, 10,
            8, 10, 11,
            // left face
            12, 13, 14,
            12, 14, 15,
            // bottom face
            16, 17, 18,
            16, 18, 19,
            // back face
            20, 21, 22,
            20, 22, 23
        };

        VertexLayout vertexLayout;

        // Postion
        vertexLayout.elements.push_back({
            VertexElement::PositionIndex,
            3,
            GL_FLOAT,
            0
            });
        // Color
        vertexLayout.elements.push_back({
            VertexElement::ColorIndex,
            3,
            GL_FLOAT,
            sizeof(float) * 3
            });
        // UV
        vertexLayout.elements.push_back({
            VertexElement::UVIndex,
            2,
            GL_FLOAT,
            sizeof(float) * 6
            });
        // Normal
        vertexLayout.elements.push_back({
            VertexElement::NormalIndex,
            3,
            GL_FLOAT,
            sizeof(float) * 8
            });
        vertexLayout.stride = sizeof(float) * 11;

        auto result = std::make_shared<Mesh>(vertexLayout, vertices, indices);

        return result;
    }

    std::shared_ptr<Mesh> Mesh::CreateSphere(float _radius, float _sectors, float _stacks)
    {
        std::vector<float> vertices((_stacks + 1) * (_sectors + 1) * 8);
        for (int i = 0; i <= _stacks; ++i)
        {
            float stackAngle = PI / 2.0f - static_cast<float>(i) * (PI / static_cast<float>(_stacks)); // From -π/2 to π/2
            float xy = _radius * cosf(stackAngle); // x-y plane radius at this stack
            float z = _radius * sinf(stackAngle);  // z coordinate

            for (int j = 0; j <= _sectors; ++j) {
                float sectorAngle = static_cast<float>(j) * (2.0f * PI / static_cast<float>(_sectors)); // From 0 to 2π

                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);

                size_t vertexStart = (i * (_sectors + 1) + j) * 8;
                // Position
                vertices[vertexStart] = x;
                vertices[vertexStart + 1] = y;
                vertices[vertexStart + 2] = z;

                // Normal (normalized position vector)
                float length = sqrtf(x * x + y * y + z * z);
                vertices[vertexStart + 3] = x / length;
                vertices[vertexStart + 4] = y / length;
                vertices[vertexStart + 5] = z / length;

                // UV coordinates
                vertices[vertexStart + 6] = static_cast<float>(j) / static_cast<float>(_sectors);
                vertices[vertexStart + 7] = static_cast<float>(i) / static_cast<float>(_stacks);
            }
        }

        // Generate indices
        std::vector<unsigned int> indices;
        for (int i = 0; i < _stacks; ++i)
        {
            int k1 = i * (_sectors + 1);
            int k2 = k1 + _sectors + 1;

            for (int j = 0; j < _sectors; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                if (i != (_stacks - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }

        VertexLayout vertexLayout;

        // Postion
        vertexLayout.elements.push_back({
            VertexElement::PositionIndex,
            3,
            GL_FLOAT,
            0
            });
        // Normal
        vertexLayout.elements.push_back({
            VertexElement::NormalIndex,
            3,
            GL_FLOAT,
            sizeof(float) * 3
            });
        // UV
        vertexLayout.elements.push_back({
            VertexElement::UVIndex,
            2,
            GL_FLOAT,
            sizeof(float) * 6
            });
        vertexLayout.stride = sizeof(float) * 8;

        auto result = std::make_shared<Mesh>(vertexLayout, vertices, indices);

        return result;
    }


    // PRIVATE ---------------------------------------------------------------------------------------------------------

}




















