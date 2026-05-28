
#include "Texture.h"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------


    Texture::Texture(const int _width, const int _height, const int _channels, const uChar *_data)
        : m_width(_width), m_height(_height), m_channels(_channels)
    {
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_2D, m_textureID);

        // load image into gpu
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height,
            0, GL_RGB, GL_UNSIGNED_BYTE, _data);

        // generate mipMap
        glGenerateMipmap(GL_TEXTURE_2D);

        // Handle texture wrapping - ST same as UV
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Filtering, how the texture scales when its drawn smaller or lager than resolution
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    Texture::~Texture()
    {
        // Free GPU memory
        if (m_textureID > 0)
        {
            glDeleteTextures(1, &m_textureID);
        }
    }

    GLuint Texture::GetID() const
    {
        return m_textureID;
    }
}
