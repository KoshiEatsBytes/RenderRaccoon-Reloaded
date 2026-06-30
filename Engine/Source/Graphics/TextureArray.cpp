
#include <stb_image.h>
#include <filesystem>

#include "TextureArray.h"
#include "Engine.h"
#include "Helpers/Printer.hpp"

namespace RR
{
    // PUBLIC ----------------------------------------------------------------------------------------------------------

    TextureArray::TextureArray()
    = default;

    TextureArray::~TextureArray()
    {
        if (m_textureID > 0)
        {
            glDeleteTextures(1, &m_textureID);
        }
    }

    std::shared_ptr<TextureArray> TextureArray::Load(const std::vector<std::string>& _layerPaths)
    {
        // Check if any valid path has been given
        if (_layerPaths.empty())
        {
            Warn("[TEXARRAY - LOAD] Could not load TexArray, empty texture paths");
            return nullptr;
        }

        auto& fileSys = Engine::GetInstance().GetFileSystem();

        // TexArray details
        const int layerCount = static_cast<int>(_layerPaths.size());
        int baseWidth  = 0;
        int baseHeight = 0;

        // Create texture array, upload and expand per texture
        GLuint id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // for colors avg
        std::vector<vec3> avgColors;

        for (int i = 0; i < layerCount; i++)
        {
            // Check if file is valid, if not abort
            auto fullPath = fileSys.GetAssetFolder() / _layerPaths[i];
            if (!std::filesystem::exists(fullPath))
            {
                Warn("[TEXARRAY - LOAD] File not found: ", fullPath.string());
                //cleanup
                glDeleteTextures(1, &id);
                return nullptr;
            }

            // per picture w h c
            int width, height, channels;
            uChar* data = stbi_load(fullPath.string().c_str(), &width, &height,
            &channels, 4); // lock at 4 channels

            // check if image loaded correctly
            if (!data)
            {
                Warn("[TEXARRAY - LOAD] Failed to decode: '", fullPath.string(), "'");
                InfoLog(stbi_failure_reason());
                glDeleteTextures(1, &id);
                return nullptr;
            }

            // first image allocates stack and sets base resolution
            if (i == 0)
            {
                baseWidth  = width;
                baseHeight = height;
                // allocate but do not fill yet
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
                    baseWidth, baseHeight, layerCount, 0, GL_RGBA,
                    GL_UNSIGNED_BYTE, nullptr);
            }
            else if (width != baseWidth || height != baseHeight)
            {
                // If one of the texture does not share dimensions,
                // discard entire array and return null
                Warn("[TEXARRAY - LOAD] Size mismatch on '", _layerPaths[i],
                     "' (", width, "x", height, " vs ", baseWidth, "x", baseHeight, ")");

                stbi_image_free(data);
                glDeleteTextures(1, &id);
                return nullptr;
            }

            // Load raw image data into GPU buffer
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                i, baseWidth, baseHeight, 1, GL_RGBA,
                GL_UNSIGNED_BYTE, data);

            vec3 sum {0.0f};
            const int texels = width * height;
            for (int pix = 0; pix < texels; ++pix)
            {
                sum.r += static_cast<float>(data[pix * 4 + 0]);
                sum.g += static_cast<float>(data[pix * 4 + 1]);
                sum.b += static_cast<float>(data[pix * 4 + 2]);
            }
            avgColors.push_back(sum / (static_cast<float>(texels) * 255.0f));

            stbi_image_free(data);
        }

        // generate texture mipmaps
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T,     GL_REPEAT);

        // create tex array object
        std::shared_ptr<TextureArray> result = std::make_shared<TextureArray>();
        result->m_textureID  = id;
        result->m_layerCount = layerCount;
        result->m_width      = baseWidth;
        result->m_height     = baseHeight;
        result->m_avgColors  = std::move(avgColors);

        return result;
    }

    const std::vector<vec3>& TextureArray::GetAvgColors() const
    {
        return m_avgColors;
    }

    vec3 TextureArray::GetAvgColor(int _index) const
    {
        return m_avgColors[_index];
    }

    GLuint TextureArray::GetTextureID() const
    {
        return m_textureID;
    }

    int TextureArray::GetLayerCount() const
    {
        return m_layerCount;
    }
}
