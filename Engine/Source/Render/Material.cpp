
#include "Material.h"

#include <nlohmann/json.hpp>
#include "Engine.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture.h"
#include "Graphics/TextureArray.h"
#include "Helpers/Printer.hpp"

using nJson = nlohmann::json;

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Material::Material()
    = default;

    Material::~Material()
    = default;

    /**
     * @brief Binds the shader and sets all the uniforms
     */
    void Material::Bind()
    {
        if (!m_shaderProgram)
        {
            Warn("[BINDING - MATERIAL] Tried to bind INVALID shader program to material");
            return;
        }

        // Bind shader
        m_shaderProgram->Bind();

        // Set 1f uniforms
        for (auto& [name, val]: m_floatParams)
        {
            m_shaderProgram->SetUniform(name, val);
        }

        // set 2f uniforms
        for (auto& [name, pair] : m_float2Params)
        {
            m_shaderProgram->SetUniform(name, pair.first, pair.second);
        }

        // set 3f uniforms
        for (auto& [name, vec] : m_float3Params)
        {
            m_shaderProgram->SetUniform(name, vec);
        }

        // Set textures
        for (auto& [name, texture] : m_textures)
        {
            m_shaderProgram->SetTexture(name, texture.get());
        }

        // Set Texture array
        for (auto& [name, texArray] : m_textureArrays)
        {
            m_shaderProgram->SetTextureArray(name, texArray.get());
        }
    }

    std::shared_ptr<Material> Material::Load(const std::string& _path)
    {
        auto& fileSys = Engine::GetInstance().GetFileSystem();
        auto& graphicsAPI = Engine::GetInstance().GetGraphicsAPI();
        auto contents = fileSys.LoadAssetFileText(_path);

        if (contents.empty())
        {
            Warn("[MATERIAL - LOAD] File not found: '", _path, "'");
            return nullptr;
        }

        // Materials are stored as Jsons, parse it when loading
        nJson json;
        try
        {
            json = nJson::parse(contents);
        }
        catch (const nJson::parse_error& e)
        {
            // Check if files are valid
            Error("[MATERIAL - LOAD] Failed to parse JSON in '", _path, "' - ", e.what());
            return nullptr;
        }

        std::shared_ptr<Material> result;

        // SHADERS //////////////////////////////////////////////////////////////////////////
        if (json.contains("Shaders"))
        {
            const auto shaderObj = json["Shaders"];
            std::string vertexPath = shaderObj.value("Vertex", "");
            std::string fragmentPath = shaderObj.value("Fragment", "");

            // check if valid path
            if (vertexPath.empty() || fragmentPath.empty())
            {
                Warn("[MATERIAL - LOAD] Missing vertex or fragment shader path in '", _path, "'");
                return nullptr;
            }

            auto vertexSrc = fileSys.LoadAssetFileText(vertexPath);
            auto fragmentSrc = fileSys.LoadAssetFileText(fragmentPath);

            // create shader program
            auto shaderProgram = graphicsAPI.CreateShaderProgram(vertexSrc, fragmentSrc);
            if (!shaderProgram)
            {
                Warn("[MATERIAL - LOAD] Shader Program generation has FAILED when loading a material");
                return nullptr;
            }

            // Init material
            result = std::make_shared<Material>();
            result->SetShaderProgram(shaderProgram);
        }
        else
        {
            // No shader block found
            Warn("[MATERIAL - LOAD] '", _path, "' is missing a Shaders block");
            return nullptr;
        }

        // RENDER STATE /////////////////////////////////////////////////////////////////////
        result->SetBackfaceCull(json.value("BackfaceCull", false));
        result->SetDepthTest   (json.value("DepthTest",    true));
        result->SetBlend       (json.value("Blend",        false));
        result->SetDepthWrite  (json.value("DepthWrite",   true));

        // PARAMS ///////////////////////////////////////////////////////////////////////////
        if (json.contains("Params"))
        {
            auto paramsObj = json["Params"];

            // Float 1 param loop
            if (paramsObj.contains("Float"))
            {
                for (auto& param : paramsObj["Float"])
                {
                    std::string name = param.value("Name", "");
                    float v0 = param.value("V0", 0.0f);
                    result->SetParam(name, v0);
                }
            }

            // Float 2 param loop
            if (paramsObj.contains("Float2"))
            {
                for (auto& param : paramsObj["Float2"])
                {
                    std::string name = param.value("Name", "");
                    float v0 = param.value("V0", 0.0f);
                    float v1 = param.value("V1", 0.0f);
                    result->SetParam(name, v0, v1);
                }
            }

            // Float 3 param loop
            if (paramsObj.contains("Float3"))
            {
                for (auto& param : paramsObj["Float3"])
                {
                    std::string name = param.value("Name", "");
                    float v0 = param.value("V0", 0.0f);
                    float v1 = param.value("V1", 0.0f);
                    float v2 = param.value("V2", 0.0f);
                    result->SetParam(name, vec3(v0, v1, v2));
                }
            }

            // Textures param loop
            if (paramsObj.contains("Textures"))
            {
                for (auto& param : paramsObj["Textures"])
                {
                    std::string name = param.value("Name", "");
                    std::string texPath = param.value("Path", "");
                    auto texture = Texture::Load(texPath);

                    // does texture loads correctly?
                    if (!texture)
                    {
                        Warn("[MATERIAL - LOAD] Skipping texture '", name, "' in '", _path, "' - failed to load");
                        continue;
                    }

                    result->SetParam(name, texture);
                }
            }

            // Texture array param loop
            if (paramsObj.contains("TextureArrays"))
            {
                for (auto& param : paramsObj["TextureArrays"])
                {
                    std::string name = param.value("Name", "");
                    const auto& layers = param["Layers"];

                    // Get texture array paths
                    std::vector<std::string> texPaths;
                    for (auto& layer : layers)
                    {
                        texPaths.push_back(layer.value("Path", ""));
                    }

                    // Load tex array and discard if not valid
                    auto arrayTex = TextureArray::Load(texPaths);
                    if (!arrayTex)
                    {
                        Warn("[MATERIAL - LOAD] Skipping texture array '", name, "' in '", _path, "' - failed to load");
                        continue;
                    }
                    result->SetParam(name, arrayTex);

                    // Per layer tint
                    const int layerCount = arrayTex->GetLayerCount();
                    for (int i = 0; i < layerCount; i++)
                    {
                        vec3 tint(1.0f);

                        // if that specific entry contrains a tint
                        if (i < static_cast<int>(layers.size()) && layers[i].contains("Tint"))
                        {
                            auto t = layers[i]["Tint"];
                            tint = vec3(t[0].get<float>(),
                                        t[1].get<float>(),
                                        t[2].get<float>());
                        }

                        result->SetParam("uTint[" + std::to_string(i) + "]", tint);
                        result->SetParam("uAvgColor[" + std::to_string(i) + "]", arrayTex->GetAvgColor(i));
                    }
                }
            }
        }

        return result;
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    void Material::SetShaderProgram(const std::shared_ptr<ShaderProgram>& _shaderProgram)
    {
        if (!_shaderProgram)
        {
            Error("[MATERIAL] Tried setting an INVALID shader program to a material");
            return;
        }

        m_shaderProgram = _shaderProgram;
    }

    void Material::SetDepthTest(const bool _enabled)
    {
        m_depthTest = _enabled;
    }

    bool Material::GetDepthTest() const
    {
        return m_depthTest;
    }

    void Material::SetBackfaceCull(const bool _enabled)
    {
        m_backfaceCull = _enabled;
    }

    bool Material::GetBackfaceCull() const
    {
        return m_backfaceCull;
    }

    void Material::SetBlend(bool _enabled)
    {
        m_blend = _enabled;
    }

    bool Material::GetBlend() const
    {
        return m_blend;
    }

    void Material::SetDepthWrite(bool _enabled)
    {
        m_depthWrite = _enabled;
    }

    bool Material::GetDepthWrite() const
    {
        return m_depthWrite;
    }

    ShaderProgram* Material::GetShaderProgram() const
    {
        return m_shaderProgram.get();
    }

    std::shared_ptr<TextureArray> Material::GetTextureArray(const std::string& _name) const
    {
        if (!m_textureArrays.contains(_name))
        {
            Warn("[MATERIAL] Tried getting a non existent texture array: '", "'");
            return nullptr;
        }

        return m_textureArrays.at(_name);
    }

    void Material::SetParam(const std::string &_name, const float _v0)
    {
        m_floatParams[_name] = _v0;
    }

    void Material::SetParam(const std::string& _name, float _v0, float _v1)
    {
        m_float2Params[_name] = {_v0, _v1};
    }

    void Material::SetParam(const std::string& _name, const vec3& _v0)
    {
        m_float3Params[_name] = _v0;
    }

    void Material::SetParam(const std::string& _name, const std::shared_ptr<Texture>& _texture)
    {
        if (!_texture)
        {
            Error("[MATERIAL] Tried setting an INVALID texture to a material");
            return;
        }

        m_textures[_name] = _texture;
    }

    void Material::SetParam(const std::string& _name, const std::shared_ptr<TextureArray>& _texArray)
    {
        if (!_texArray)
        {
            Error("[MATERIAL] Tried setting an INVALID texture array to a material");
            return;
        }

        m_textureArrays[_name] = _texArray;
    }
}




