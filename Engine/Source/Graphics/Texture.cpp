
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
        GLenum channelFormat = 0;

        switch (_channels)
        {
            case 1:
                channelFormat = GL_RED;
                break;
            case 2:
                channelFormat = GL_RG;
                break;
            case 3:
                channelFormat = GL_RGB;
                break;
            case 4:
                channelFormat = GL_RGBA;
                break;

            default:
                Warn("[TEXTURE - INIT - CHANNELS] Current texture channels are out of scope, defaulting to RGBA");
                channelFormat = GL_RGBA;
                break;
        }

        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // load image into gpu
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, channelFormat, _width, _height,
            0, channelFormat, GL_UNSIGNED_BYTE, _data);

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

    GLuint Texture::GetID() const
    {
        return m_textureID;
    }
}



















