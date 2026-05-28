
#pragma once

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

        fSysPath GetExecutableFolder() const;
        fSysPath GetAssetFolder() const;
    };
}
