
#pragma once
#include <memory>
#include <unordered_map>

namespace RR
{
    class Texture;
    class ShaderProgram;

    class Material
    {
    public:
        Material();
        ~Material();

        void Bind();

        void SetShaderProgram(const std::shared_ptr<ShaderProgram>& _shaderProgram);
        ShaderProgram* GetShaderProgram() const;

        void SetParam(const std::string& _name, float _v0);
        void SetParam(const std::string& _name, float _v0, float _v1);
        void SetParam(const std::string& _name, const std::shared_ptr<Texture>& _texture);

    private:
        std::shared_ptr<ShaderProgram> m_shaderProgram;
        std::unordered_map<std::string, float> m_floatParams;
        std::unordered_map<std::string, std::pair<float, float>> m_float2Params;
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    };
}


