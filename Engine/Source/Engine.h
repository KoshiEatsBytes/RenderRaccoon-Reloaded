
#pragma once
#include <memory>
#include <chrono>

#include "Input/InputManager.h"
#include "Graphics/GraphicsAPI.h"
#include "Graphics/Texture.h"
#include "Render/RenderQueue.h"
#include "Scene/Scene.h"
#include "FileSystem/FileSystem.h"
#include "Physics/PhysicsManager.h"
#include "Helpers/ApplicationData.h"
#include "SceneManager.h"

// fw dc to allow window to be available
// as part of the singleton
struct GLFWwindow;

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

        // Get/Sets
        //void SetApp(SceneManager* _app);
        //SceneManager* GetApp() const;

        void SetScene(Scene* _scene);
        Scene* GetScene() const;

        void SetShouldClose(bool _close);
        bool GetShouldClose() const;

        InputManager& GetInputManager();
        GraphicsAPI& GetGraphicsAPI();
        RenderQueue& GetRenderQueue();
        FileSystem& GetFileSystem();
        TextureManager& GetTextureManager();
        PhysicsManager& GetPhysicsManager();
        ApplicationData& GetAppData();

    private:
        //std::unique_ptr<Scene> m_application;
        std::chrono::steady_clock::time_point m_lastTimePoint;
        std::unique_ptr<Scene> m_currentScene;

        bool m_shouldClose = false;
        GLFWwindow* m_window = nullptr;
        InputManager m_inputManager;
        GraphicsAPI m_graphicsAPI;
        RenderQueue m_renderQueue;
        FileSystem m_fileSystem;
        TextureManager m_textureManager;
        PhysicsManager m_physicsManager;
        //ApplicationData m_appData;
    };
}


