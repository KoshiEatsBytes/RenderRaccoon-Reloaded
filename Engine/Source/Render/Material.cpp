
#include "Material.h"

#include <nlohmann/json.hpp>
#include "Engine.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture.h"
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

        // Set textures
        for (auto& [name, texture]: m_textures)
        {
            m_shaderProgram->SetTexture(name, texture.get());
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

    ShaderProgram* Material::GetShaderProgram() const
    {
        return m_shaderProgram.get();
    }

    void Material::SetParam(const std::string &_name, const float _v0)
    {
        m_floatParams[_name] = _v0;
    }

    void Material::SetParam(const std::string& _name, float _v0, float _v1)
    {
        m_float2Params[_name] = {_v0, _v1};
    }

    void Material::SetParam(const std::string &_name, const std::shared_ptr<Texture>& _texture)
    {
        if (!_texture)
        {
            Error("[MATERIAL] Tried setting an INVALID texture to a material");
            return;
        }

        m_textures[_name] = _texture;
    }
}




