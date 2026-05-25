
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

    // PRIVATE ---------------------------------------------------------------------------------------------------------


}
