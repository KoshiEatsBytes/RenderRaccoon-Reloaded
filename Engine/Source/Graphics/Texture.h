
#pragma once
#include <GL/glew.h>
#include <unordered_map>

#include "../Helpers/Types.h"

namespace RR
{
    class Texture
    {
    public:
        Texture(int _width, int _height, int _channels, const uChar* _data);
        ~Texture();

        void Init(int _width, int _height, int _channels, const uChar* _data);
        static std::shared_ptr<Texture> Load(const std::string& _path);

        GLuint GetTextureID() const;

    private:
        // Texture data
        int m_width = 0;
        int m_height = 0;
        int m_channels = 0;

        GLuint m_textureID = 0;
    };

    class TextureManager
    {
        TextureManager();
        ~TextureManager();

    public:
        // Only engine can construct this class
        friend class Engine;

        // Delete copy
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

        // Delete move
        TextureManager(TextureManager&&) noexcept = delete;
        TextureManager& operator=(TextureManager&&) noexcept = delete;

        std::shared_ptr<Texture> GetOrLoadTexture(const std::string& _path);

    private:
        std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    };
}
