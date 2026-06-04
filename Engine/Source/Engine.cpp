
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <LinearMath/btThreads.h>

#include "Engine.h"
#include "ApplicationManager.h"
#include "Helpers/Printer.hpp"
#include "Scene/Component.h"
#include "Scene/Components/CameraComponent.h"

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    Engine::Engine()
    = default;

    Engine::~Engine()
    {
        if (m_btScheduler != nullptr)
        {
            delete m_btScheduler;
        }
    }

    /**
     * @brief Updates input manager with keyboard inputs
     * @param _window GLFW window pointer
     * @param _key keyboard key identifier
     * @param _scanCode unknown
     * @param _action If pressed / released
     * @param _mods any modifiers (such as shift)
     */
    void Engine::KeyCallBack(GLFWwindow* _window, int _key, int _scanCode, int _action, int _mods)
    {
        auto& inputManager = GetInstance().GetInputManager();

        if (_action == GLFW_PRESS)
        {
            inputManager.SetKeyPressed(_key, true);
        }
        else if (_action == GLFW_RELEASE)
        {
            inputManager.SetKeyPressed(_key, false);
        }
    }

    /**
     * @brief Updates input manager with mouse button inputs
     * @param _window GLFW window pointer
     * @param _button Mouse button identifier
     * @param _action If pressed / Released
     * @param _mods any modifiers
     */
    void Engine::MouseButtonCallBack(GLFWwindow* _window, int _button, int _action, int _mods)
    {
        auto& inputManager = GetInstance().GetInputManager();

        if (_action == GLFW_PRESS)
        {
            inputManager.SetMouseButtonPressed(_button, true);
        }
        else if (_action == GLFW_RELEASE)
        {
            inputManager.SetMouseButtonPressed(_button, false);
        }
    }

    /**
     * @brief Updates input manager with new cursor pos
     * @param _window GLFW window pointer
     * @param xPos new cursor xPos
     * @param yPos new cursor yPos
     */
    void Engine::CursorPositionCallBack(GLFWwindow* _window, double xPos, double yPos)
    {
        auto& inputManager = GetInstance().GetInputManager();

        // push back last frame pos
        inputManager.SetMousePositionOld(inputManager.GetMousePositionCurrent());

        const vec2 currentPos = {static_cast<float>(xPos), static_cast<float>(yPos)};
        inputManager.SetMousePositionCurrent(currentPos);

        inputManager.SetMousePositionChanged(true);
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Engine& Engine::GetInstance()
    {
        static Engine instance;
        return instance;
    }

    bool Engine::Init(const int& _width, const int& _height, const std::string& _name)
    {
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
        glfwSetMouseButtonCallback(m_window, MouseButtonCallBack);
        glfwSetCursorPosCallback(m_window, CursorPositionCallBack);
        // sticks cursor so application
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glfwMakeContextCurrent(m_window);

        // Enable physics library multithreading
        m_btScheduler = btCreateDefaultTaskScheduler();
        btSetTaskScheduler(m_btScheduler);
        m_btScheduler->setNumThreads(8);//static_cast<int>(m_btScheduler->getNumThreads() * 0.5f));
        Log("[PHYSICS] Scheduler: ", m_btScheduler->getName(), " | threads: ", m_btScheduler->getNumThreads());

        if (glewInit() != GLEW_OK)
        {
            Error("[INITIALIZATION] Failed to initialize GLEW library.");
            glfwTerminate();
            return false;
        }

        if (!m_graphicsAPI.Init())
        {
            Error("[INITIALIZATION] Failed to initialize RR Graphics API");
            glfwTerminate();
            return false;
        }

        if (!m_appManager.Init())
        {
            Error("[INITIALIZATION] Failed to initialize RR Application Manager");
            glfwTerminate();
            return false;
        }

        return true;
    }

    void Engine::Launch()
    {
        // Get last time point before application start to compute dt
        m_lastTimePoint = std::chrono::steady_clock::now();

        // Collect camera and light data
        CameraData camData;
        std::vector<LightData> lights;

        // Main application loop
        while (!glfwWindowShouldClose(m_window) &&
               !m_shouldClose)
        {
            glfwPollEvents();

            // compute delta time
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
            m_lastTimePoint = now;

            m_appManager.PreUpdate(deltaTime);
            // Physics step
            m_appManager.PhysicsUpdate(deltaTime);

            m_appManager.Update(deltaTime);

            // Drawing
            m_graphicsAPI.SetClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            m_graphicsAPI.ClearBuffers();

            // Dynamically adjusts the aspect ratio
            int width = 0;
            int height = 0;
            glfwGetWindowSize(m_window, &width, &height);
            float aspect = static_cast<float>(width) / static_cast<float>(height);

            if (const auto scene = m_appManager.GetActiveScene())
            {
                if (const auto camObj = scene->GetMainCamera())
                {
                    // logic for matrices
                    if (auto camComponent = camObj->FindComponentByType<CameraComponent>())
                    {
                        camData.viewMatrix = camComponent->GetViewMatrix();
                        camData.projMatrix = camComponent->GetProjectionMatrix(aspect);
                        camData.position = camObj->GetWorldPosition();
                    }
                }

                lights = scene->GetLights();
            }

            m_renderQueue.Draw(m_graphicsAPI, camData, lights);
            glfwSwapBuffers(m_window);

            // Mouse moved this frame
            m_inputManager.SetMousePositionChanged(false);

            // Late Update before new frame
            m_appManager.LateUpdate(deltaTime);
        }
    }

    void Engine::Destroy()
    {
        m_appManager.Destroy();

        // Clean-up glfw
        glfwTerminate();
        m_window = nullptr;

        Log("Closing down, bye bye!");
    }

    // GETTER / SETTERS ------------------------------------------------------------------------------------------------

    void Engine::SetShouldClose(bool _close)
    {
        m_shouldClose = _close;
    }

    bool Engine::GetShouldClose() const
    {
        return m_shouldClose;
    }

    Scene* Engine::GetScene() const
    {
        return m_appManager.GetActiveScene();
    }

    // Re-Enable if needed in the future
    // PhysicsManager* Engine::GetPhysicsManager() const
    // {
    //     return m_appManager.GetActiveScene()->GetPhysicsManager();
    // }

    ApplicationManager& Engine::GetAppManager()
    {
        return m_appManager;
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

    FileSystem& Engine::GetFileSystem()
    {
        return m_fileSystem;
    }

    TextureManager & Engine::GetTextureManager()
    {
        return m_textureManager;
    }

    ApplicationData& Engine::GetAppData()
    {
        return m_appData;
    }
}
