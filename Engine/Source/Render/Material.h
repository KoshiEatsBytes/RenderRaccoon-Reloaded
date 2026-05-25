
#pragma once
#include <memory>
#include <unordered_map>

namespace RR
{
    class ShaderProgram;

    class Material
    {
    public:
        Material();
        ~Material();

        void SetShaderProgram(const std::shared_ptr<ShaderProgram>& _shaderProgram);
        void SetParam(const std::string& _name, float _val);
        void Bind();

    private:
        std::shared_ptr<ShaderProgram> m_shaderProgram;
        std::unordered_map<std::string, float> m_floatParams;
    };
}


