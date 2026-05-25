
#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "Printer.hpp"

namespace RR
{
    /**
     * @brief Simple wrapper to read shader files from disk
     */
    class ShaderLoader
    {
    public:
        [[nodiscard]] static std::string LoadFromFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
            {
                Warn("[SHADER] Failed to open shader at: ", path.string());
                return "";
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();

            if (file.bad())
            {
                Warn("[SHADER] File is corrupted and/or contains error/s, discarding. File: ", path.string());
                return "";
            }

            return buffer.str();
        }
    };
}
