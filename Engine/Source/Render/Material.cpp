
#include "Material.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture.h"
#include "Helpers/Printer.hpp"

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




