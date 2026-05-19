
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Engine.h"
#include "Application.h"

namespace rr
{
    Engine::Engine()
    = default;

    Engine::~Engine()
    = default;

    bool Engine::Init(const int& _width, const int& _height, const std::string& _name)
    {
        if (!m_application)
        {
            // No valid application
            warn("Tried initializing invalid application");
            return false;
        }

        // Initializes window
        if (!glfwInit())
        {
            error("Failed to initialize GLFW library.");
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        m_window = glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);

        if (m_window == nullptr)
        {
            error("Failed to create window.");
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(m_window);

        if (glewInit() != GLEW_OK)
        {
            error("Failed to initialize GLEW library.");
            glfwTerminate();
            return false;
        }

        return m_application->Init();
    }

    void Engine::Launch()
    {
        if (!m_application)
        {
            warn("Tried launching invalid application");
            return;
        }

        m_lastTimePoint = std::chrono::steady_clock::now();

        // Main application loop
        while (!m_application->ShouldClose() &&
               !glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();

            // compute delta time
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
            m_lastTimePoint = now;

            m_application->Update(deltaTime);

            // place holder
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

            return;
        }

        warn("Tried destroying invalid application");
    }

    void Engine::SetApp(Application* app)
    {
        m_application.reset(app);
    }

    Application* Engine::GetApp() const
    {
        return m_application.get();
    }
}
