
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

    std::shared_ptr<Mesh> Mesh::CreateCube()
    {
        // Triangle made with vertices
        // Color of each vertices is on the right
        std::vector<float> vertices
        {
            // Front face
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            // Top face
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            // Right face
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0, 0.0f, 0.0f,

            // Left face
            -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0, 0.0f, 0.0f,

            // Bottom face
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,

            // Back face
            -0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f,
        };

        // Instead of re-rendering vertices, use them from the existent array
        std::vector<unsigned int> indeces =
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
            //left face
            12, 13, 14,
            12, 14, 15,
            // bottom face
            16, 17, 18,
            16, 18, 19,
            // back face
            20, 21, 22,
            20, 22, 23
        };

        RR::VertexLayout vertexLayout;

        // Position
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

        // Stride
        vertexLayout.stride = sizeof(float) * 11;

        auto result = std::make_shared<RR::Mesh>(vertexLayout, vertices, indeces);

        return result;
    }

    // /**
    //  * @brief Loads a GLTF mesh from path
    //  * @param _path relative path to the gltf file
    //  * @return shared_ptr of mesh
    //  */
    // std::shared_ptr<Mesh> Mesh::LoadGLTF(const std::string& _path)
    // {
    //     auto& fileSys = Engine::GetInstance().GetFileSystem();
    //     //auto& graphicsAPI = Engine::GetInstance().GetGraphicsAPI();
    //     auto contents = fileSys.LoadAssetFileText(_path);
    //
    //     if (contents.empty())
    //     {
    //         Warn("[MESH - LOADING - GLTF] File not found: '", _path, "'");
    //         return nullptr;
    //     }
    //
    //     // Load model with CGLTF
    //     cgOptions options = {};
    //     cgData* data = nullptr;
    //     cgResult res = cgltf_parse(&options, contents.data(), contents.size(), &data);
    //
    //     // if prase unsuccessful
    //     if (res != cgltf_result_success)
    //     {
    //         Warn("[MESH - LOADING - GLTF] GLTF File was not parsed successfully at: '", _path, "'");
    //         return nullptr;
    //     }
    //
    //     auto fullPath = fileSys.GetAssetFolder() / _path;
    //
    //     // Load binary buffer
    //     res = cgltf_load_buffers(&options, data, fullPath.remove_filename().string().c_str());
    //
    //     // check if ok
    //     if (res != cgltf_result_success)
    //     {
    //         Warn("[MESH - LOADING - GLTF] GLTF failed to load buffer successfully of file at path: '", _path, "'");
    //         cgltf_free(data);
    //         return nullptr;
    //     }
    //
    //     // if both previous steps succeeded, data holds everything we need for a model
    //
    //     std::shared_ptr<Mesh> result = nullptr;
    //
    //     // mi -> Mesh Index
    //     for (cgSize mi = 0; mi < data->meshes_count; mi++)
    //     {
    //         auto& mesh = data->meshes[mi];
    //
    //         // pi -> Primitive Index
    //         for (cgSize pi = 0; pi < mesh.primitives_count; pi++)
    //         {
    //             auto& primitive = mesh.primitives[pi];
    //
    //             // skip if not triangles, for now
    //             if (primitive.type != cgltf_primitive_type_triangles)
    //             {
    //                 continue;
    //             }
    //
    //             // process primitive loading
    //             VertexLayout vertexLayout;
    //
    //             /**
    //              *  GLTFS primitives have a list of attributes, access them with accessors
    //              *
    //              *  Pos is set to index 0
    //              *  Color is set to index 1
    //              *  UV is set to index 2
    //              *  Normal is set to index 3
    //              */
    //             cgAccessor* accessors[4] = {nullptr, nullptr, nullptr, nullptr};
    //
    //
    //             // ai -> Attribute Index
    //             for (cgSize ai = 0; ai < primitive.attributes_count; ai++)
    //             {
    //                 auto& attr = primitive.attributes[ai];
    //                 auto acc = attr.data;
    //
    //                 // accessor is null, skip
    //                 if (!acc) continue;
    //
    //                 // Create vertex element
    //                 VertexElement element;
    //                 element.type = GL_FLOAT;
    //
    //                 // check attribute type and act accordingly
    //                 switch (attr.type)
    //                 {
    //                     case cgltf_attribute_type_position:
    //                     {
    //                         accessors[VertexElement::PositionIndex] = acc;
    //                         element.index = VertexElement::PositionIndex;
    //                         element.size = 3;
    //                     }
    //                     break;
    //
    //                     case cgltf_attribute_type_texcoord:
    //                     {
    //                         // Target only first layer of UVs
    //                         if (attr.index != 0)
    //                         {
    //                             continue;
    //                         }
    //
    //                         accessors[VertexElement::UVIndex] = acc;
    //                         element.index = VertexElement::UVIndex;
    //                         element.size = 2;
    //                     }
    //                     break;
    //
    //                     case cgltf_attribute_type_color:
    //                     {
    //                         // For now only use color channel at Index 0
    //                         if (attr.index != 0)
    //                         {
    //                             continue;
    //                         }
    //
    //                         accessors[VertexElement::ColorIndex] = acc;
    //                         element.index = VertexElement::ColorIndex;
    //                         element.size = 3;
    //                     }
    //                     break;
    //
    //                     case cgltf_attribute_type_normal:
    //                     {
    //                         accessors[VertexElement::NormalIndex] = acc;
    //                         element.index = VertexElement::NormalIndex;
    //                         element.size = 3;
    //                     }
    //                     break;
    //
    //                     default:
    //                         continue;
    //                 }
    //
    //                 // For each attribute fill vertex element and push it to vLayout
    //                 if (element.size > 0)
    //                 {
    //                     element.offset = vertexLayout.stride;
    //                     vertexLayout.stride += element.size * sizeof(float);
    //                     vertexLayout.elements.push_back(element);
    //                 }
    //             }
    //
    //             // once layout is ready, allocate buffer
    //
    //             // If there is nothing to draw continue with the next primitive
    //             if (!accessors[VertexElement::PositionIndex]) continue;
    //
    //             auto vertexCount = accessors[VertexElement::PositionIndex]->count;
    //
    //             std::vector<float> vertices;
    //             vertices.resize((vertexLayout.stride / sizeof(float)) * vertexCount);
    //
    //             // vi -> Vertex Index
    //             for (cgSize vi = 0; vi < vertexCount; vi++)
    //             {
    //                 for (auto& el : vertexLayout.elements)
    //                 {
    //                     // check if accessor exist for this element
    //                     if (!accessors[el.index]) continue;
    //
    //                     const auto index = (vi * vertexLayout.stride + el.offset) / sizeof(float);
    //
    //                     float* outData = &vertices[index];
    //                     CGLTFLib::ReadFloats(accessors[el.index], vi, outData, el.size);
    //                 }
    //             }
    //
    //             // check if primitive has an accessor
    //             if (primitive.indices)
    //             {
    //                 const auto indexCount = primitive.indices->count;
    //                 std::vector<uint32> indices(indexCount);
    //
    //                 for (cgSize i = 0; i < indexCount; i++)
    //                 {
    //                     indices[i] = CGLTFLib::ReadIndex(primitive.indices, i);
    //                 }
    //
    //                 result = std::make_shared<Mesh>(vertexLayout, vertices, indices);
    //             }
    //             else // if primitive does not have index accessor
    //             {
    //                 result = std::make_shared<Mesh>(vertexLayout, vertices);
    //             }
    //
    //             if (result)
    //             {
    //                 break;
    //             }
    //         }
    //
    //         if (result)
    //         {
    //             break;
    //         }
    //     }
    //
    //     // free data
    //     cgltf_free(data);
    //
    //     return result;
    // }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

}




















