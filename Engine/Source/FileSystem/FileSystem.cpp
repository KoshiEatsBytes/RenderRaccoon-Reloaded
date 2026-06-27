
#include <fstream>
#include <algorithm>

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

    std::ofstream FileSystem::OpenOutputFile(const std::string& _relativePath, bool _binary) const
    {
        const fSysPath fullPath = GetOutputFolder() / _relativePath;
        const auto mode = _binary ? (std::ios::out | std::ios::binary) : std::ios::out;

        std::error_code error;
        std::filesystem::create_directories(fullPath.parent_path(), error);
        if (error)
        {
            Error("[FILESYSTEM - WRITE] Could not create directory '", fullPath.parent_path().string(),
                  "': ", error.message());
        }


        std::ofstream out(fullPath, mode);
        if (!out) Warn("[FILESYSTEM - WRITE] Could not open '", fullPath, "' for writing");
        return out;
    }

    std::vector<fSysPath> FileSystem::ListAssetFiles(const std::string& _subfolder,
        const std::vector<std::string>& _extensions) const
    {
        return ListFilesIn(GetAssetFolder(),  _subfolder, _extensions);
    }

    std::vector<fSysPath> FileSystem::ListOutputFiles(const std::string& _subfolder,
        const std::vector<std::string>& _extensions) const
    {
        return ListFilesIn(GetOutputFolder(),  _subfolder, _extensions);
    }

    std::string FileSystem::LoadOutputFileText(const std::string& _relativePath)
    {
        auto buffer = LoadFile(GetOutputFolder() / _relativePath);
        return std::string(buffer.begin(), buffer.end());
    }

    bool FileSystem::DeleteOutputFile(const std::string &_relativePath) const
    {
        const fSysPath fullPath = GetOutputFolder() / _relativePath;

        std::error_code error;
        const bool removed = std::filesystem::remove(fullPath, error);

        if (error)
        {
            Warn("[FILESYSTEM - DELETE] Could not delete '", fullPath, "': ", error.message());
            return false;
        }

        // flase w/o error means was already gone
        return removed;
    }

    fSysPath FileSystem::GetExecutableFolder() const
    {
#if defined _WIN32
        wChar buffer[MAX_PATH];
        GetModuleFileNameW(NULL, buffer, MAX_PATH);
        return fSysPath(buffer).remove_filename();
#elif defined (__APPLE__)
        uInt32 size = 0;
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

    fSysPath FileSystem::GetOutputFolder() const
    {
        return std::filesystem::weakly_canonical(GetExecutableFolder() / "Output");
    }

    std::vector<fSysPath> FileSystem::ListFilesIn(const fSysPath &_root, const std::string& _subfolder,
        const std::vector<std::string>& _extensions) const
    {
        std::vector<fSysPath> result;
        const fSysPath dir  = _root / _subfolder;

        if (!std::filesystem::exists(dir)) return result;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (!entry.is_regular_file()) continue;
            if (!_extensions.empty())
            {
                std::string extension = entry.path().extension().string();
                std::ranges::transform(extension, extension.begin(), ::tolower);
                if (std::ranges::find(_extensions, extension) == _extensions.end()) continue;
            }
            result.push_back(std::filesystem::relative(entry.path(), _root));
        }
        // Make result deterministic
        std::ranges::sort(result);
        return result;
    }
}










































