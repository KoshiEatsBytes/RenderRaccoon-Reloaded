
#pragma once
#include <memory>
#include <chrono>

#include "Helpers/Printer.hpp"
#include "Input/InputManager.h"

struct GLFWwindow;

namespace RR
{
    // fw dec for application
    class Application;

    class Engine
    {
        Engine();
        ~Engine();

        // Delete copy & copy assignment
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // Delete move & move assignment
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        // handles keyCallBack
        static void KeyCallBack(GLFWwindow* window, int key,
            int scanCode, int action, int mods);

    public:
        // Singleton - return engine instance
        static Engine& GetInstance();

        bool Init(const int& _width, const int& _height,
            const std::string& _name);
        void Launch();
        void Destroy();

        void SetApp(Application* app);
        Application* GetApp() const;
        InputManager& GetInputManager();

    private:
        std::unique_ptr<Application> m_application;
        std::chrono::steady_clock::time_point m_lastTimePoint;

        GLFWwindow* m_window = nullptr;
        InputManager m_inputManager;
    };
}


