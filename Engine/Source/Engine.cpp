
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Engine.h"
#include "Application.h"
#include "Helpers/Printer.hpp"
#include "Scene/Component.h"
#include "Scene/Components/CameraComponent.h"

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    Engine::Engine()
    = default;

    Engine::~Engine()
    = default;

    void Engine::KeyCallBack(GLFWwindow* _window, int _key, int _scanCode, int _action, int _mods)
    {
        auto& inputManager = GetInstance().GetInputManager();

        // Automatically updates input manager from keyboard input
        if (_action == GLFW_PRESS)
        {
            inputManager.SetKeyPressed(_key, true);
        }
        else if (_action == GLFW_RELEASE)
        {
            inputManager.SetKeyPressed(_key, false);
        }
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Engine& Engine::GetInstance()
    {
        static Engine instance;
        return instance;
    }

    bool Engine::Init(const int& _width, const int& _height, const std::string& _name)
    {
        if (!m_application)
        {
            // No valid application
            Warn("[INITIALIZATION] Tried initializing invalid application");
            return false;
        }

        // Initializes window
        if (!glfwInit())
        {
            Error("[INITIALIZATION] Failed to initialize GLFW library.");
            return false;
        }

        // Set openGL version to current 3.3.0 CORE
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // create and return window as *
        m_window = glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);

        if (m_window == nullptr)
        {
            Error("[INITIALIZATION] Failed to create window.");
            glfwTerminate();
            return false;
        }

        // Input polling
        glfwSetKeyCallback(m_window, KeyCallBack);

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(0); // Turn off vertical sync

        if (glewInit() != GLEW_OK)
        {
            Error("[INITIALIZATION] Failed to initialize GLEW library.");
            glfwTerminate();
            return false;
        }

        if (m_application->Init())
        {
            Success("[INITIALIZATION] Application Initialized Correctly.");
            return true;
        }

        // If init fails terminate engine and log to console
        Error("[INITIALIZATION] Failed to initialize application, terminating.");
        InfoLog("This is not a fault with the engine module but with application: '", typeid(*m_application).name(), "'");
        glfwTerminate();
        return false;
    }

    void Engine::Launch()
    {
        if (!m_application)
        {
            Warn("Tried launching invalid application");
            return;
        }

        // Get last time point before application start to compute dt
        m_lastTimePoint = std::chrono::steady_clock::now();

        // Main application loop
        while (!m_application->GetShouldClose() &&
               !glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();

            // compute delta time
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
            m_lastTimePoint = now;

            m_application->Update(deltaTime);

            // Drawing
            m_graphicsAPI.SetClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            m_graphicsAPI.ClearBuffers();

            // Collect camera data
            CameraData camData;
            if (m_currentScene)
            {
                if (auto camObj = m_currentScene->GetMainCamera())
                {
                    // logic for matrices
                    if (auto camComponent = camObj->GetComponent<CameraComponent>())
                    {
                        camData.viewMatrix = camComponent->GetViewMatrix();
                        camData.projMatrix = camComponent->GetProjectionMatrix();
                    }
                }
            }

            m_renderQueue.Draw(m_graphicsAPI, camData);
            glfwSwapBuffers(m_window);
        }
    }

    void Engine::Destroy()
    {
        if (m_application)
        {
            m_application->Destroy();
            m_application.reset();

            // Clean-up glfw
            glfwTerminate();
            m_window = nullptr;

            Log("Closing down, bye bye!");

            return;
        }

        Warn("Tried destroying invalid application");
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    void Engine::SetApp(Application* _app)
    {
        m_application.reset(_app);
    }

    Application* Engine::GetApp() const
    {
        return m_application.get();
    }

    void Engine::SetScene(Scene* _scene)
    {
        m_currentScene.reset(_scene);
    }

    Scene* Engine::GetScene() const
    {
        return m_currentScene.get();
    }

    InputManager& Engine::GetInputManager()
    {
        return m_inputManager;
    }

    GraphicsAPI& Engine::GetGraphicsAPI()
    {
        return m_graphicsAPI;
    }

    RenderQueue& Engine::GetRenderQueue()
    {
        return m_renderQueue;
    }
}
