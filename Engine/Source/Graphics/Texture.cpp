
#include <stb_image.h>

#include "Texture.h"
#include "Engine.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Texture::Texture(const int _width, const int _height, const int _channels, const uChar *_data)
        : m_width(_width), m_height(_height), m_channels(_channels)
    {
        Init(_width, _height, _channels, _data);
    }

    Texture::~Texture()
    {
        // Free GPU memory
        if (m_textureID > 0)
        {
            glDeleteTextures(1, &m_textureID);
        }
    }

    void Texture::Init(int _width, int _height, int _channels, const uChar *_data)
    {
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_2D, m_textureID);

        GLint internalFormat = GL_RGB;
        GLenum format = GL_RGB;

        switch (_channels)
        {
            case 1:
                internalFormat = GL_RED;
                format = GL_RED;
                break;
            case 2:
                internalFormat = GL_RG;
                format = GL_RG;
                break;
            case 3:
                internalFormat = GL_RGB;
                format = GL_RGB;
                break;
            case 4:
                internalFormat = GL_RGBA;
                format = GL_RGBA;
                break;
            default:
                Warn("[TEXTURE - INIT - CHANNELS] Current texture channels are out of scope, defaulting to RGBA");
                internalFormat = GL_RGBA;
                format = GL_RGBA;
                break;
        }

        // load image into gpu
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _width, _height,
            0, format, GL_UNSIGNED_BYTE, _data);

        // generate mipMap
        glGenerateMipmap(GL_TEXTURE_2D);

        // Handle texture wrapping - ST same as UV
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Filtering, how the texture scales when its drawn smaller or lager than resolution
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    std::shared_ptr<Texture> Texture::Load(const std::string& _path)
    {
        int width, height, channels;

        // get fullpath
        auto& fileSys = RR::Engine::GetInstance().GetFileSystem();
        auto fullPath = fileSys.GetAssetFolder() / _path;

        // Check if file is present
        if (!std::filesystem::exists(fullPath))
        {
            Warn("[TEXTURE - LOAD] File not found: ", fullPath.string());
            return nullptr;
        }

        uChar* data = stbi_load(fullPath.string().c_str(), &width, &height,
            &channels, 0);

        // Check if data is not corrupted
        if (data)
        {
            std::shared_ptr<Texture> result = std::make_shared<Texture>(width, height, channels, data);
            stbi_image_free(data);

            return result;
        }

        Warn("[TEXTURE - LOAD] Failed to decode: '", fullPath.string(), "'");
        InfoLog(stbi_failure_reason());
        return nullptr;
    }

    GLuint Texture::GetTextureID() const
    {
        return m_textureID;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TEXTURE MAMAGER
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // PRIVATE ---------------------------------------------------------------------------------------------------------

    TextureManager::TextureManager()
    = default;

    TextureManager::~TextureManager()
    = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    std::shared_ptr<Texture> TextureManager::GetOrLoadTexture(const std::string& _path)
    {
        auto it = m_textures.find(_path);

        // if texture is found return it
        if (it != m_textures.end())
        {
            return it->second;
        }

        // create it if not found
        auto texture = Texture::Load(_path);
        m_textures[_path] = texture;
        return texture;
    }
}



















