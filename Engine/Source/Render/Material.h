
#pragma once
#include <memory>
#include <unordered_map>

#include "Helpers/Types.h"

namespace RR
{
    class Texture;
    class TextureArray;
    class ShaderProgram;

    class Material
    {
    public:
        Material();
        ~Material();

        void Bind();

        static std::shared_ptr<Material> Load(const std::string& _path);

        void SetShaderProgram(const std::shared_ptr<ShaderProgram>& _shaderProgram);

        void SetDepthTest(bool _enabled);
        bool GetDepthTest() const;
        void SetBackfaceCull(bool _enabled);
        bool GetBackfaceCull() const;
        void SetBlend(bool _enabled);
        bool GetBlend() const;

        ShaderProgram* GetShaderProgram() const;
        std::shared_ptr<TextureArray> GetTextureArray(const std::string& _name) const;

        void SetParam(const std::string& _name, float _v0);
        void SetParam(const std::string& _name, float _v0, float _v1);
        void SetParam(const std::string& _name, const vec3& _v0);
        void SetParam(const std::string& _name, const std::shared_ptr<Texture>& _texture);
        void SetParam(const std::string& _name, const std::shared_ptr<TextureArray>& _texArray);

    private:
        bool m_backfaceCull = false;
        bool m_depthTest    = true;
        bool m_blend        = false;

        // Param maps
        std::shared_ptr<ShaderProgram> m_shaderProgram;
        std::unordered_map<std::string, float> m_floatParams;
        std::unordered_map<std::string, std::pair<float, float>> m_float2Params;
        std::unordered_map<std::string, vec3> m_float3Params;
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
        std::unordered_map<std::string, std::shared_ptr<TextureArray>> m_textureArrays;
    };
}


