
#pragma once
#include <string>

namespace RR
{
    class Engine;
    class ApplicationData
    {
        ApplicationData() = default;
        ~ApplicationData() = default;

    public:
        // Owned by engine
        friend class Engine;
        ApplicationData(const ApplicationData&) = delete;
        ApplicationData& operator=(const ApplicationData&) = delete;
        ApplicationData(ApplicationData&&) noexcept = delete;
        ApplicationData& operator=(ApplicationData&&) noexcept = delete;

        // Device info — gathered once by the Engine at startup
        std::string  gpuName;        
        std::string  cpuName;       
        unsigned int coreCount = 0;  
    };
}


