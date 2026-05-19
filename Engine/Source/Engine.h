
#pragma once
#include <memory>
#include <chrono>

#include "Printer.hpp"

namespace rr
{
    // fw dec for application
    class Application;

    class Engine
    {
    public:
        Engine();
        ~Engine();

        bool Init() const;
        void Launch();
        void Destroy();

        void SetApp(Application* app);
        Application* GetApp() const;

    private:
        std::unique_ptr<Application> _application;
        std::chrono::steady_clock::time_point _lastTimePoint;
    };
}


