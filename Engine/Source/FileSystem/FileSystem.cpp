
#include "FileSystem.h"
#include "config.h"

#if defined _WIN32
#include <windows.h>
#elif defined (__APPLE__)
#include <mach-o/dyld.h>
#elif defined (__LINUX__)
#include <unistd.h>
#include <limits.h>
#endif

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    FileSystem::FileSystem()
    = default;

    FileSystem::~FileSystem()
    = default;

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    fSysPath FileSystem::GetExecutableFolder() const
    {
#if defined _WIN32
        wChar buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        return fSysPath(buffer).remove_filename();
#elif defined (__APPLE__)
        uint32 size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string tmp(size, '\0');
        _NSGetExecutablePath(tmp.data(), &size);
        return std::filesystem::weakly_canonical(fSysPath(tmp)).remove_filename();
#elif defined (__LINUX__)
        return std::filesystem::weakly_canonical(std::filesystem::read_symlink("proc/self/exe")).remove_filename();
#else
        return std::filesystem::current_path();
#endif
    }

    fSysPath FileSystem::GetAssetFolder() const
    {
#if defined (ASSETS_ROOT)
        auto dirPath = fSysPath(std::string(ASSETS_ROOT));
        if (std::filesystem::exists(dirPath))
        {
            return dirPath;
        }
#endif
        return std::filesystem::weakly_canonical(GetExecutableFolder() / "Assets");
    }
}
