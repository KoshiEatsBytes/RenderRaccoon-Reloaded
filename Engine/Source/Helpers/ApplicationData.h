
#pragma once

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

        // DATA HERE!!
        int hello = {54};
    };
}


