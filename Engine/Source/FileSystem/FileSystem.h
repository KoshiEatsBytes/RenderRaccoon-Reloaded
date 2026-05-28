
#pragma once
#include <vector>

#include "Types.h"

namespace RR
{
    class FileSystem
    {
        FileSystem();
        ~FileSystem();

    public:
        friend class Engine;

        // Delete copy
        FileSystem(const FileSystem&) = delete;
        FileSystem& operator=(const FileSystem&) = delete;

        // Delete move
        FileSystem(FileSystem&&) noexcept = delete;
        FileSystem& operator=(FileSystem&&) noexcept = delete;

        std::vector<char> LoadFile(const fSysPath& _path);
        std::vector<char> LoadAssetFile(const std::string& _relativePath);

        std::string LoadAssetFileText(const std::string &_relativePath);

        fSysPath GetExecutableFolder() const;
        fSysPath GetAssetFolder() const;
    };
}
