
#pragma once
#include <chrono>

#include "ApplicationManager.h"
#include "Input/InputManager.h"
#include "Graphics/GraphicsAPI.h"
#include "Graphics/Texture.h"
#include "Render/RenderQueue.h"
#include "FileSystem/FileSystem.h"
#include "Helpers/ApplicationData.h"

// fw dc to allow window to be available
// as part of the singleton
struct GLFWwindow;
class btITaskScheduler;

namespace RR
{
    class Engine
    {
        Engine();
        ~Engine();

        // Callback keyboard input
        static void KeyCallBack(GLFWwindow* _window, int _key,
            int _scanCode, int _action, int _mods);

        // Callback mouse buttons
        static void MouseButtonCallBack(GLFWwindow* _window,
            int _button, int _action, int _mods);

        // Cursor position callback
        static void CursorPositionCallBack(GLFWwindow* _window,
            double xPos, double yPos);

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

        void SetShouldClose(bool _close);
        bool GetShouldClose() const;

        Scene* GetScene() const;
        //PhysicsManager* GetPhysicsManager() const;
        ApplicationManager& GetAppManager();
        InputManager& GetInputManager();
        GraphicsAPI& GetGraphicsAPI();
        RenderQueue& GetRenderQueue();
        FileSystem& GetFileSystem();
        TextureManager& GetTextureManager();
        ApplicationData& GetAppData();

    private:
        std::chrono::steady_clock::time_point m_lastTimePoint;

        bool m_shouldClose = false;
        GLFWwindow* m_window = nullptr;
        btITaskScheduler* m_btScheduler = nullptr;

        ApplicationManager m_appManager;
        InputManager m_inputManager;
        GraphicsAPI m_graphicsAPI;
        RenderQueue m_renderQueue;
        FileSystem m_fileSystem;
        TextureManager m_textureManager;
        ApplicationData m_appData;
    };
}


