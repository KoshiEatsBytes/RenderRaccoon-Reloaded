
#pragma once
#include <GL/glew.h>

#include "Types.h"

namespace RR
{
    class Texture
    {
    public:
        Texture(int _width, int _height, int _channels, const uChar* _data);
        ~Texture();

        void Init(int _width, int _height, int _channels, const uChar* _data);
        static std::shared_ptr<Texture> Load(const std::string& _path);

        GLuint GetID() const;

    private:
        // Texture data
        int m_width = 0;
        int m_height = 0;
        int m_channels = 0;

        GLuint m_textureID = 0;
    };
}
