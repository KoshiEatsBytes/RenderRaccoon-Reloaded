
#ifndef RENDERRACCOON_RELOADED_SHADERIO_H
#define RENDERRACCOON_RELOADED_SHADERIO_H

#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Helper
{
    class ShaderIO
    {
    public:
        [[nodiscard]] static std::string LoadFromFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
            {
                throw std::runtime_error("Failed to open shader file: " + path.string());
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();

            if (file.bad())
            {
                throw std::runtime_error("Failed to read shader file: " + path.string());
            }

            return buffer.str();
        }
    };
}

#endif //RENDERRACCOON_RELOADED_SHADERIO_H
