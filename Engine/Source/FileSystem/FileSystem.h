
#pragma once
#include <vector>
#include <fstream>

#include "../Helpers/Types.h"

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
        std::string LoadAssetFileText(const std::string& _relativePath);
        std::ofstream OpenOutputFile(const std::string& _relativePath, bool _binary = true) const;

        std::vector<fSysPath> ListAssetFiles(const std::string& _subfolder,
            const std::vector<std::string>& _extensions = {}) const;

        std::vector<fSysPath> ListOutputFiles(const std::string& _subfolder,
            const std::vector<std::string>& _extensions = {}) const;

        std::string LoadOutputFileText(const std::string& _relativePath);
        bool DeleteOutputFile(const std::string& _relativePath) const;
        bool WriteOutputTextFile(const std::string& _relativePath, const std::string& _text) const;

        static std::string MakeTimestamp();

        fSysPath GetExecutableFolder() const;
        fSysPath GetAssetFolder() const;
        fSysPath GetOutputFolder() const;

    private:
        std::vector<fSysPath> ListFilesIn(const fSysPath& _root, const std::string& _subfolder,
                                  const std::vector<std::string>& _extensions) const;
    };
}
