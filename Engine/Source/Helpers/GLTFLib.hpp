
#pragma once
#include <cgltf.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Graphics/Texture.h"
#include "../Graphics/VertexLayout.h"
#include "../Render/Material.h"
#include "../Render/Mesh.h"
#include "../Scene/Components/MeshComponent.h"
#include "../Scene/Components/AnimationComponent.h"
#include "AnimationStructs.h"

using cgAccessor = cgltf_accessor;
using cgOptions = cgltf_options;
using cgResult = cgltf_result;
using cgData = cgltf_data;
using cgSize = cgltf_size;
using cgUint = cgltf_uint;
using cgBool = cgltf_bool;
using cgNode = cgltf_node;

namespace RR
{
    struct CGLTFLib
    {
        static bool ReadFloats(const cgAccessor* _acc, cgSize _i, float* _out, int _n)
        {
            std::fill_n(_out, _n, 0.0f);
            return cgltf_accessor_read_float(_acc, _i, _out, _n) == 1;
        }

        static uInt32 ReadIndex(const cgAccessor* _acc, cgSize _i)
        {
            cgUint out = 0;
            const cgBool ok = cgltf_accessor_read_uint(_acc, _i, &out, 1);
            return ok ? static_cast<uInt32>(out) : 0;
        }

        static float ReadScalar(cgAccessor* _acc, cgSize _index)
        {
            float res = 0.0f;
            cgltf_accessor_read_float(_acc, _index, &res, 1);
            return res;
        }

        static vec3 ReadVec3(cgAccessor* _acc, cgSize _index)
        {
            vec3 res;
            cgltf_accessor_read_float(_acc, _index, glm::value_ptr(res), 3);
            return res;
        }

        static quat ReadQuat(cgAccessor* _acc, cgSize _index)
        {
            float res[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            cgltf_accessor_read_float(_acc, _index, res, 4);
            return quat(res[3], res[0], res[1], res[2]);
        }

        static void ReadTimes(cgAccessor* _acc, std::vector<float>& _outTimes)
        {
            _outTimes.resize(_acc->count);
            for (cgSize i = 0; i < _acc->count; i++)
            {
                _outTimes[i] = ReadScalar(_acc, i);
            }
        }

        static void ReadOutputVec3(cgAccessor* _acc, std::vector<vec3>& _outValues)
        {
            _outValues.resize(_acc->count);
            for (cgSize i = 0; i < _acc->count; i++)
            {
                _outValues[i] = ReadVec3(_acc, i);
            }
        }

        static void ReadOutputQuat(cgAccessor* _acc, std::vector<quat>& _outValues)
        {
            _outValues.resize(_acc->count);
            for (cgSize i = 0; i < _acc->count; i++)
            {
                _outValues[i] = ReadQuat(_acc, i);
            }
        }

        static GameObject* LoadGLTF(const std::string& _path, Scene* _scene)
        {
            auto& fileSys = Engine::GetInstance().GetFileSystem();

            // Load file as text
            auto contents = fileSys.LoadAssetFileText(_path);

            if (contents.empty())
            {
                Warn("[GLTF - LOADING] File not found: '", _path, "'");
                return nullptr;
            }

            // Load model with CGLTF
            cgOptions options = {};
            cgData* data = nullptr;
            cgResult res = cgltf_parse(&options, contents.data(), contents.size(), &data);

            // if prase unsuccessful
            if (res != cgltf_result_success)
            {
                Warn("[LOADING - GLTF] GLTF File was not parsed successfully at: '", _path, "'");
                return nullptr;
            }

            auto fullPath = fileSys.GetAssetFolder() / _path;
            auto fullFolderPath = fullPath.remove_filename();
            auto relativeFolderPath = fSysPath(_path).remove_filename();

            // Load binary buffer
            res = cgltf_load_buffers(&options, data, fullFolderPath.string().c_str());

            // check if ok
            if (res != cgltf_result_success)
            {
                Warn("[GLTF - LOADING] GLTF failed to load buffer successfully of file at path: '", _path, "'");
                cgltf_free(data);
                return nullptr;
            }

            std::string modelName = "CGLTFLoadedModel" + std::to_string(m_resultIndex);
            auto resultObject = _scene->CreateObject(modelName);
            m_resultIndex++;

            // GLTF files can have scenes, for now only grab scene no. 1 (index 0)
            // also check if there is a valid scene
            if (data->scenes_count == 0)
            {
                Warn("[GLTF - LOADING] No scenes found in: '", _path, "'");
                cgltf_free(data);
                return nullptr;
            }

            auto modelScene = &data->scenes[0];

            for (cgSize i = 0; i < modelScene->nodes_count; i++)
            {
                auto node = modelScene->nodes[i];

                ParseNode(node, resultObject, relativeFolderPath);
            }

            // Parse all animation
            std::vector<std::shared_ptr<AnimationClip>> clips;

            // ai -> Animation Index
            for (cgSize ai = 0; ai < data->animations_count; ai++)
            {
                auto& anim = data->animations[ai];

                // Create animation clip
                auto clip = std::make_shared<AnimationClip>();
                clip->name = anim.name ? anim.name : "NoNameAnim";
                clip->duration = 0;

                // Populate animation clip
                std::unordered_map<cgNode*, sizeT> trackIndexOf;

                // Helper lambda for track creation
                auto GetOrCreateTrack = [&](cgNode* _node) -> TransformTrack&
                {
                    auto it = trackIndexOf.find(_node);

                    // if clip registered return it
                    if (it != trackIndexOf.end())
                    {
                        return clip->tracks[it->second];
                    }

                    // if not found create new track
                    TransformTrack track;
                    track.targetName = _node->name;
                    clip->tracks.push_back(track);
                    sizeT idx = clip->tracks.size() - 1;
                    trackIndexOf[_node] = idx;
                    return clip->tracks[idx];
                };

                // ci -> Channel Index
                for (cgSize ci = 0; ci < anim.channels_count; ci++)
                {
                    auto& channel = anim.channels[ci];
                    auto sampler = channel.sampler;

                    if (!channel.target_node || !sampler ||
                        !sampler->input || !sampler->output)
                    {
                        continue;
                    }

                    std::vector<float> timeStamps;
                    ReadTimes(sampler->input, timeStamps);

                    auto& track = GetOrCreateTrack(channel.target_node);

                    switch (channel.target_path)
                    {
                        case cgltf_animation_path_type_translation:
                        {
                            std::vector<vec3> values;
                            ReadOutputVec3(sampler->output, values);
                            track.positions.resize(timeStamps.size());

                            for (sizeT i = 0; i < timeStamps.size(); i++)
                            {
                                track.positions[i].timeStamp = timeStamps[i];
                                track.positions[i].value = values[i];
                            }
                        }
                        break;

                        case cgltf_animation_path_type_rotation:
                        {
                            std::vector<quat> values;
                            ReadOutputQuat(sampler->output, values);
                            track.rotations.resize(timeStamps.size());

                            for (sizeT i = 0; i < timeStamps.size(); i++)
                            {
                                track.rotations[i].timeStamp = timeStamps[i];
                                track.rotations[i].value = values[i];
                            }
                        }
                        break;

                        case cgltf_animation_path_type_scale:
                        {
                            std::vector<vec3> values;
                            ReadOutputVec3(sampler->output, values);
                            track.scales.resize(timeStamps.size());

                            for (sizeT i = 0; i < timeStamps.size(); i++)
                            {
                                track.scales[i].timeStamp = timeStamps[i];
                                track.scales[i].value = values[i];
                            }
                        }
                        break;

                        default:
                            break;
                    }

                    clip->duration = std::max(clip->duration, timeStamps.back());
                }

                clips.push_back(std::move(clip));
            }

            // if there is at least one animation prepare an animation component
            if (!clips.empty())
            {
                auto animComp = new AnimationComponent();
                resultObject->AddComponent(animComp);

                // Add clips to anim comp
                for (auto& clip : clips)
                {
                    animComp->RegisterClip(clip->name, clip);
                }
            }

            Success("[GLTF - SUCCESS] GLTF Model Loaded correctly!");
            cgltf_free(data);

            return resultObject;
        }

        static void ParseNode(cgNode* _node, GameObject* _parent, const fSysPath& _folder)
        {
            auto& textureManager = Engine::GetInstance().GetTextureManager();

            std::string nodeName = _node->name ? _node->name : "UnnamedNode";
            GameObject* object = _parent->GetScene()->CreateObject(nodeName, _parent);

            /**
             *  GLTF nodes can store transformation in 2 ways
             *  - With a MATRIX
             *  - With separate components
             */

            // if it has matrix, decompose matrix into separate components
            if (_node->has_matrix)
            {
                auto mat = glm::make_mat4(_node->matrix);

                // Decompose gltf matrix into separate values
                vec3 translation, scale, skew;
                vec4 perspective;
                quat rotation;
                glm::decompose(mat, scale, rotation, translation, skew, perspective);

                object->SetPosition(translation);
                object->SetRotation(rotation);
                object->SetScale(scale);
            }
            // if it does not have a matrix specify components separately
            else
            {
                // Position
                if (_node->has_translation)
                {
                    object->SetPosition(vec3(
                    _node->translation[0],
                    _node->translation[1],
                    _node->translation[2]
                    ));
                }
                // Rotation
                if (_node->has_rotation)
                {
                    object->SetRotation(quat(
                        _node->rotation[3],
                        _node->rotation[0],
                        _node->rotation[1],
                        _node->rotation[2]
                        ));
                }
                // Scale
                if (_node->has_scale)
                {
                    object->SetScale(vec3(
                        _node->scale[0],
                        _node->scale[1],
                        _node->scale[2]
                        ));
                }
            }

            // If the node has a mesh
            if (_node->mesh)
            {
                // pi -> Primitive Index
                for (cgSize pi = 0; pi < _node->mesh->primitives_count; pi++)
                {
                    auto& primitive = _node->mesh->primitives[pi];

                    // skip if not triangles, for now
                    if (primitive.type != cgltf_primitive_type_triangles)
                    {
                        continue;
                    }

                    // process primitive loading
                    VertexLayout vertexLayout;

                    /**
                     *  GLTFS primitives have a list of attributes, access them with accessors
                     *
                     *  Pos is set to index 0
                     *  Color is set to index 1
                     *  UV is set to index 2
                     *  Normal is set to index 3
                     */
                    cgAccessor* accessors[4] = {nullptr, nullptr, nullptr, nullptr};

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

                            case cgltf_attribute_type_normal:
                            {
                                accessors[VertexElement::NormalIndex] = acc;
                                element.index = VertexElement::NormalIndex;
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

                    // Calculate vertices and compose array
                    auto vertexCount= accessors[VertexElement::PositionIndex]->count;
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
                            ReadFloats(accessors[el.index], vi, outData, el.size);
                        }
                    }

                    std::shared_ptr<Mesh> mesh;

                    // check if primitive has an accessor
                    // If so create mesh
                    if (primitive.indices)
                    {
                        const auto indexCount = primitive.indices->count;
                        std::vector<uInt32> indices(indexCount);

                        for (cgSize i = 0; i < indexCount; i++)
                        {
                            indices[i] = ReadIndex(primitive.indices, i);
                        }

                        mesh = std::make_shared<Mesh>(vertexLayout, vertices, indices);
                    }
                    else // if primitive does not have index accessor
                    {
                        mesh = std::make_shared<Mesh>(vertexLayout, vertices);
                    }

                    /**
                     *  GLTF MATERIALS ARE PBR BASED
                     *
                     *  Come with 2 workflows
                     *  - Metallic Roughness
                     *  - Specular Glossiness
                     */

                    auto mat = std::make_shared<Material>();
                    mat->SetShaderProgram(Engine::GetInstance().GetGraphicsAPI().GetDefaultShaderProgram());

                    // if primitive mat exists
                    if (primitive.material)
                    {
                        auto gltfMat = primitive.material;

                        // If has metallic roughness
                        if (gltfMat->has_pbr_metallic_roughness)
                        {
                            auto pbr = gltfMat->pbr_metallic_roughness;
                            auto texture = pbr.base_color_texture.texture;

                            // check if texture exists and has an image
                            if (texture && texture->image)
                            {
                                if (texture->image->uri)
                                {
                                    auto path = _folder / std::string(texture->image->uri);
                                    auto tex = textureManager.GetOrLoadTexture(path.string());

                                    mat->SetParam("baseColorTexture", tex);
                                }
                            }
                        }
                        else if (gltfMat->has_pbr_specular_glossiness)
                        {
                            auto pbr = gltfMat->pbr_specular_glossiness;
                            auto texture = pbr.diffuse_texture.texture;

                            // check if texture exists and has an image
                            if (texture && texture->image)
                            {
                                if (texture->image->uri)
                                {
                                    auto path = _folder / std::string(texture->image->uri);
                                    auto tex = textureManager.GetOrLoadTexture(path.string());

                                    mat->SetParam("baseColorTexture", tex);
                                }
                            }
                        }
                    }

                    object->AddComponent(new MeshComponent(mat, mesh));
                }
            }

            // iterate over children and process them 1by1
            // ci -> Child Index
            for (cgSize ci = 0; ci < _node->children_count; ci++)
            {
                ParseNode(_node->children[ci], object, _folder);
            }
        }

    private:
        inline static sizeT m_resultIndex = 0;
    };
}




