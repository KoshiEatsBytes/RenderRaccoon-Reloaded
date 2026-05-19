
#pragma once
#include <memory>
#include <chrono>

#include "Printer.hpp"

struct GLFWwindow;

namespace rr
{
    // fw dec for application
    class Application;

    class Engine
    {
    public:
        Engine();
        ~Engine();

        bool Init(const int& _width, const int& _height,
                  const std::string& _name);
        void Launch();
        void Destroy();

        void SetApp(Application* app);
        Application* GetApp() const;

    private:
        std::unique_ptr<Application> m_application;
        std::chrono::steady_clock::time_point m_lastTimePoint;

        GLFWwindow* m_window = nullptr;
    };
}


