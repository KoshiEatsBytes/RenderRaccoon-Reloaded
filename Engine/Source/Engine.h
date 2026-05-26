
#pragma once
#include <memory>
#include <chrono>

#include "Input/InputManager.h"
#include "Graphics/GraphicsAPI.h"
#include "Render/RenderQueue.h"

// fw dc to allow window to be available
// as part of the singleton
struct GLFWwindow;

namespace RR
{
    // fw dec for application
    class Application;

    class Engine
    {
        Engine();
        ~Engine();

        // handles input via callback
        static void KeyCallBack(GLFWwindow* _window, int _key,
                                int _scanCode, int _action, int _mods);

    public:
        // Delete copy
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // Delete move
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        // Singleton - return engine instance
        static Engine& GetInstance();

        bool Init(const int& _width, const int& _height,
            const std::string& _name);
        void Launch();
        void Destroy();

        void SetApp(Application* app);
        Application* GetApp() const;

        InputManager& GetInputManager();
        GraphicsAPI& GetGraphicsAPI();
        RenderQueue& GetRenderQueue();

    private:
        std::unique_ptr<Application> m_application;
        std::chrono::steady_clock::time_point m_lastTimePoint;

        GLFWwindow* m_window = nullptr;
        InputManager m_inputManager;
        GraphicsAPI m_graphicsAPI;
        RenderQueue m_renderQueue;
    };
}


