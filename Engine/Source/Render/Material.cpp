
#include "Material.h"
#include "Graphics/ShaderProgram.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Material::Material()
    = default;

    Material::~Material()
    = default;

    void Material::SetShaderProgram(const std::shared_ptr<ShaderProgram> &_shaderProgram)
    {
        m_shaderProgram = _shaderProgram;
    }

    void Material::SetParam(const std::string &_name, const float &_val)
    {
        m_floatParams[_name] = _val;
    }

    // binds the shader and sets all the uniforms
    void Material::Bind()
    {
        if (!m_shaderProgram)
        {
            Warn("[MATERIAL] Tried to bind INVALID shader to material");
            return;
        }

        // Bind shader
        m_shaderProgram->Bind();

        // Iterate over parameters and set them for the shader program
        for (const auto& [key, val]: m_floatParams)
        {
            m_shaderProgram->SetUniform(key, val);
        }
    }

    // PRIVATE ---------------------------------------------------------------------------------------------------------



}
