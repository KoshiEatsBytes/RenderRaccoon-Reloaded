
#include <fstream>

#include "FileSystem.h"
#include "config.h"
#include "Helpers/Printer.hpp"

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

    std::vector<char> FileSystem::LoadFile(const fSysPath &_path)
    {
        std::ifstream file(_path, std::ios::binary | std::ios::ate);

        // if failed to open file return nothing
        if (!file.is_open())
        {
            Warn("[FILESYSTEM - LOAD FILE] File at path '", _path, "' not found or inaccessible");
            return {};
        }

        auto size = file.tellg();
        file.seekg(0);

        std::vector<char> buffer(size);

        if (!file.read(buffer.data(), size))
        {
            Warn("[FILESYSTEM - READ FILE] File at path '", _path, "' cannot be read");
            return {};
        }

        return buffer;
    }

    std::vector<char> FileSystem::LoadAssetFile(const std::string &_relativePath)
    {
        return LoadFile(GetAssetFolder() / _relativePath);
    }

    std::string FileSystem::LoadAssetFileText(const std::string &_relativePath)
    {
        auto buffer = LoadAssetFile(_relativePath);
        return std::string(buffer.begin(), buffer.end());
    }

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
        Warn("[FILESYSTEM - ASSETS] ASSETS_ROOT path not found, falling back to executable folder");
#endif
        return std::filesystem::weakly_canonical(GetExecutableFolder() / "Assets");
    }
}
