
#include <cgltf.h>
#include <string>

#include "Mesh.h"
#include "Graphics/GraphicsAPI.h"
#include "Engine.h"
#include "Helpers/Printer.hpp"

using cgAccessor = cgltf_accessor;
using cgOptions = cgltf_options;
using cgResult = cgltf_result;
using cgData = cgltf_data;
using cgSize = cgltf_size;
using cgUint = cgltf_uint;
using cgBool = cgltf_bool;

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

    std::shared_ptr<Mesh> Mesh::LoadGLTF(const std::string& _path)
    {
        auto& fileSys = Engine::GetInstance().GetFileSystem();
        //auto& graphicsAPI = Engine::GetInstance().GetGraphicsAPI();
        auto contents = fileSys.LoadAssetFileText(_path);

        if (contents.empty())
        {
            Warn("[MESH - LOADING - GLTF] File not found: '", _path, "'");
            return nullptr;
        }

        // Helper lambda for reading model floats
        auto readFloats = [](const cgAccessor* _acc, cgSize _i, float* _out, int _n)
        {
            std::fill_n(_out, _n, 0.0f);
            return cgltf_accessor_read_float(_acc, _i, _out, _n) == 1;
        };

        // Helper lambda for reading model indexes
        auto readIndex = [](const cgAccessor* _acc, cgSize _i)
        {
            cgUint out = 0;
            const cgBool ok = cgltf_accessor_read_uint(_acc, _i, &out, 1);
            return ok ? static_cast<uint32>(out) : 0;
        };

        // Load model with CGLTF
        cgOptions options = {};
        cgData* data = nullptr;
        cgResult res = cgltf_parse(&options, contents.data(), contents.size(), &data);

        // if prase unsuccessful
        if (res != cgltf_result_success)
        {
            Warn("[MESH - LOADING - GLTF] GLTF File was not parsed successfully at: '", _path, "'");
            return nullptr;
        }

        auto fullPath = fileSys.GetAssetFolder() / _path;

        // Load binary buffer
        res = cgltf_load_buffers(&options, data, fullPath.remove_filename().string().c_str());

        // check if ok
        if (res != cgltf_result_success)
        {
            Warn("[MESH - LOADING - GLTF] GLTF failed to load buffer successfully of file at path: '", _path, "'");
            cgltf_free(data);
            return nullptr;
        }

        // if both previous steps succeeded, data holds everything we need for a model

        std::shared_ptr<Mesh> result = nullptr;

        // mi -> Mesh Index
        for (cgSize mi = 0; mi < data->meshes_count; mi++)
        {
            auto& mesh = data->meshes[mi];

            // pi -> Primitive Index
            for (cgSize pi = 0; pi < mesh.primitives_count; pi++)
            {
                auto& primitive = mesh.primitives[pi];

                // skip if not triangles, for now
                if (primitive.type != cgltf_primitive_type_triangles)
                {
                    continue;
                }

                // process primitive loading
                VertexLayout vertexLayout;
                cgAccessor* accessors[3] = {nullptr, nullptr, nullptr}; // position colors and uvs

                /**
                 * GLTFS primitives have a list of attributes
                 *
                 *  Pos is set to index 1
                 *  Color is set to index 2
                 *  UV is set to index 3
                 */

                // ai -> Attribute Index
                for (cgSize ai = 0; ai < primitive.attributes_count; ai++)
                {
                    auto& attr = primitive.attributes[ai];
                    auto acc = attr.data;

                    // accessor is null, skip
                    if (!acc) continue;

                    // Create vertex element
                    VertexElement element;
                    element.type = GL_FLOAT;

                    // check attribute type and act accordingly
                    switch (attr.type)
                    {
                        case cgltf_attribute_type_position:
                        {
                            accessors[VertexElement::PositionIndex] = acc;
                            element.index = VertexElement::PositionIndex;
                            element.size = 3;
                        }
                        break;

                        case cgltf_attribute_type_texcoord:
                        {
                            // Target only first layer of UVs
                            if (attr.index != 0)
                            {
                                continue;
                            }

                            accessors[VertexElement::UVIndex] = acc;
                            element.index = VertexElement::UVIndex;
                            element.size = 2;
                        }
                        break;

                        case cgltf_attribute_type_color:
                        {
                            // For now only use color channel at Index 0
                            if (attr.index != 0)
                            {
                                continue;
                            }

                            accessors[VertexElement::ColorIndex] = acc;
                            element.index = VertexElement::ColorIndex;
                            element.size = 3;
                        }
                        break;

                        default:
                            continue;
                    }

                    // For each attribute fill vertex element and push it to vLayout
                    if (element.size > 0)
                    {
                        element.offset = vertexLayout.stride;
                        vertexLayout.stride += element.size * sizeof(float);
                        vertexLayout.elements.push_back(element);
                    }
                }

                // once layout is ready, allocate buffer

                // If there is nothing to draw continue with the next primitive
                if (!accessors[VertexElement::PositionIndex]) continue;

                auto vertexCount = accessors[VertexElement::PositionIndex]->count;

                std::vector<float> vertices;
                vertices.resize((vertexLayout.stride / sizeof(float)) * vertexCount);

                // vi -> Vertex Index
                for (cgSize vi = 0; vi < vertexCount; vi++)
                {
                    for (auto& el : vertexLayout.elements)
                    {
                        // check if accessor exist for this element
                        if (!accessors[el.index]) continue;

                        const auto index = (vi * vertexLayout.stride + el.offset) / sizeof(float);

                        float* outData = &vertices[index];
                        readFloats(accessors[el.index], vi, outData, el.size);
                    }
                }

                // check if primitive has an accessor
                if (primitive.indices)
                {
                    const auto indexCount = primitive.indices->count;
                    std::vector<uint32> indices(indexCount);

                    for (cgSize i = 0; i < indexCount; i++)
                    {
                        indices[i] = readIndex(primitive.indices, i);
                    }

                    result = std::make_shared<Mesh>(vertexLayout, vertices, indices);
                }
                else // if primitive does not have index accessor
                {
                    result = std::make_shared<Mesh>(vertexLayout, vertices);
                }

                if (result)
                {
                    break;
                }
            }

            if (result)
            {
                break;
            }
        }

        // free data
        cgltf_free(data);

        return result;
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------

}




















