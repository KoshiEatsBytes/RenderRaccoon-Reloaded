
#pragma once
#include <GL/glew.h>
#include <vector>
#include <string>
#include "Helpers/Types.h"

namespace RR
{
    class TextureArray
    {
    public:
        TextureArray();
        ~TextureArray();

        static std::shared_ptr<TextureArray> Load(
            const std::vector<std::string>& _layerPaths);

        const std::vector<vec3>& GetAvgColors() const;
        vec3 GetAvgColor(int _index) const;

        GLuint GetTextureID() const;
        int    GetLayerCount() const;

    private:

        GLuint m_textureID = 0;
        int m_layerCount   = 0;
        int m_width        = 0;
        int m_height       = 0;

        // avg color of each texture array entry
        std::vector<vec3> m_avgColors;
    };
}
