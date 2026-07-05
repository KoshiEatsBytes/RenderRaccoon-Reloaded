
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <LinearMath/btThreads.h>
#include <thread>
#include <cstring>
#include <string>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    #define RR_HAS_CPUID 1
    #if defined(_MSC_VER)
        #include <intrin.h>
    #else
        #include <cpuid.h>
    #endif
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

#include "Engine.h"
#include "ApplicationManager.h"
#include "implot.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Benchmark/BenchmarkSubSystem.h"
#include "Helpers/Printer.hpp"
#include "Scene/Component.h"
#include "Scene/Components/CameraComponent.h"

namespace
{
    std::string ReadGlString(GLenum _name)
    {
        const GLubyte* string = glGetString(_name);
        return string ? reinterpret_cast<const char*>(string) : "unknown";
    }

    // CPU brand string via CPUID (x86 only "unknown" everywhere else)
    std::string ReadCpuBrand()
    {
#if defined(RR_HAS_CPUID)
        unsigned int regs[12] = {};
    #if defined(_MSC_VER)
        int cpui[4];
        __cpuid(cpui, 0x80000000);
        if (static_cast<unsigned int>(cpui[0]) < 0x80000004u) return "unknown";
        __cpuidex(cpui, 0x80000002, 0); std::memcpy(&regs[0], cpui, 16);
        __cpuidex(cpui, 0x80000003, 0); std::memcpy(&regs[4], cpui, 16);
        __cpuidex(cpui, 0x80000004, 0); std::memcpy(&regs[8], cpui, 16);
    #else
        unsigned int maxExt = 0, ebx = 0, ecx = 0, edx = 0;
        __get_cpuid(0x80000000u, &maxExt, &ebx, &ecx, &edx);
        if (maxExt < 0x80000004u) return "unknown";
        __get_cpuid(0x80000002u, &regs[0], &regs[1], &regs[2],  &regs[3]);
        __get_cpuid(0x80000003u, &regs[4], &regs[5], &regs[6],  &regs[7]);
        __get_cpuid(0x80000004u, &regs[8], &regs[9], &regs[10], &regs[11]);
    #endif
        char brand[49];
        std::memcpy(brand, regs, 48);
        brand[48] = '\0';

        std::string s(brand);
        const auto first = s.find_first_not_of(' ');
        const auto last  = s.find_last_not_of(' ');
        return (first == std::string::npos) ? "unknown" : s.substr(first, last - first + 1);
#else
        return "unknown";
#endif
    }
}

namespace RR
{
    // PRIVATE ---------------------------------------------------------------------------------------------------------

    Engine::Engine()
    = default;

    Engine::~Engine()
    {
        m_appManager.UnloadCurrentScene();
        //if (m_btScheduler) delete m_btScheduler;
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

    void Engine::ScrollCallBack(GLFWwindow* _window, double _xOffset, double _yOffset)
    {
        auto& inputManager = GetInstance().GetInputManager();
        inputManager.SetScrollDelta(static_cast<float>(_yOffset));
    }

    // PUBLIC ----------------------------------------------------------------------------------------------------------

    Engine& Engine::GetInstance()
    {
        static Engine instance;
        return instance;
    }

    bool Engine::Init(int _width, int _height, const std::string& _name, uInt _audioChannels, uInt _fallbackChannel)
    {
#ifdef _WIN32
        // set console to utf8
        SetConsoleOutputCP(CP_UTF8);
#endif

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
        glfwSetScrollCallback(m_window, ScrollCallBack);

        glfwMakeContextCurrent(m_window);

        // Enable physics library multithreading
        m_btScheduler = btGetSequentialTaskScheduler();
        btSetTaskScheduler(m_btScheduler);
        //m_btScheduler->setNumThreads(static_cast<int>(m_btScheduler->getNumThreads() * 0.5f));
        //Log("[PHYSICS] Scheduler: ", m_btScheduler->getName(), " | threads: ", m_btScheduler->getNumThreads());

        // enable openGL experimental
        glewExperimental = GL_TRUE;

        if (glewInit() != GLEW_OK)
        {
            Error("[INITIALIZATION] Failed to initialize GLEW library.");
            glfwTerminate();
            return false;
        }

        // glew experimental may fail, flush it
        glGetError();

        if (!m_graphicsAPI.Init())
        {
            Error("[INITIALIZATION] Failed to initialize RR Graphics API");
            glfwTerminate();
            return false;
        }

        // Gather device info 
        m_appData.gpuName   = ReadGlString(GL_RENDERER);
        m_appData.cpuName   = ReadCpuBrand();
        m_appData.coreCount = std::thread::hardware_concurrency();
        Log("[INIT] Device — CPU: ", m_appData.cpuName, " | GPU: ", m_appData.gpuName,
            " | logical cores: ", m_appData.coreCount);

        // Wire up ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui::StyleColorsDark();

        // Initialize ImGui font
        ImGuiIO& io = ImGui::GetIO();
        const std::string fontPath = (m_fileSystem.GetAssetFolder() / "Fonts/JetBrainsMono-Regular.ttf").string();
        if (!io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f))
        {
            Warn("[IMGUI] font not found, falling back to ProggyClean: ", fontPath);
        }
        float dpiScale = std::clamp(static_cast<float>(_width) / 1920.0f, 0.5f, 3.0f);
        ImGui::GetStyle().FontScaleDpi = dpiScale;

        if (!ImGui_ImplGlfw_InitForOpenGL(m_window, true))
        {
            Error("[INITIALIZATION] Failed to initialize ImGui glfw for OpenGL");
            glfwTerminate();
            return false;
        }

        if (!ImGui_ImplOpenGL3_Init("#version 330"))
        {
            Error("[INITIALIZATION] Failed to initialize ImGui");
            glfwTerminate();
            return false;
        }

        if (!m_audioManager.Init(_audioChannels, _fallbackChannel))
        {
            Error("[INITIALIZATION] Failed to initialize RR Audio Engine");
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

        // containers for camera and light data
        CameraData camData;
        std::vector<LightData> lights;

        // Get benchmark tool if present
        auto* benchmark = m_appManager.GetSubSystem<BenchmarkSubSystem>();

        // Main application loop
        while (!glfwWindowShouldClose(m_window) &&
               !m_shouldClose)
        {
            glfwPollEvents();

            // ImGui early frame update
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            const ImGuiIO& io = ImGui::GetIO();
            m_inputManager.SetUICapture(io.WantCaptureKeyboard, io.WantCaptureMouse);

            // compute delta time
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
            m_lastTimePoint = now;

            // Being frame for bench
            if (benchmark) benchmark->BeginFrame(deltaTime);

            m_appManager.PreUpdate(deltaTime);
            // Physics step
            m_appManager.PhysicsUpdate(deltaTime);

            m_appManager.Update(deltaTime);

            // Drawing
            const auto scene = m_appManager.GetActiveScene();
            if (scene)
                m_graphicsAPI.SetClearColor(scene->GetSceneClearColor());
            else
                m_graphicsAPI.SetClearColor();

            // if using reversed Z draw scene into float-depth target then blit to screen
            int fbW = 0, fbH = 0;
            glfwGetFramebufferSize(m_window, &fbW, &fbH);

            // fb width and height 0 when minimized
            const bool useSceneRT = m_graphicsAPI.IsReversedZ() && fbW > 0 && fbH > 0;

            if (useSceneRT)
            {
                m_graphicsAPI.BeginSceneTarget(fbW, fbH);
            }

            m_graphicsAPI.ClearBuffers();

            if (scene)
            {
                if (const auto camObj = scene->GetMainCamera())
                {
                    // logic for matrices
                    if (auto camComponent = camObj->FindComponentByType<CameraComponent>())
                    {
                        if (camComponent->IsEnabled())
                        {
                            const float AP = GetAspectRatio();

                            camData.viewMatrix = camComponent->GetViewMatrix();
                            camData.projMatrix = camComponent->GetProjectionMatrix(AP);
                            camData.position   = camObj->GetWorldPosition();
                        }
                        else
                        {
                            camData.viewMatrix = 1.0f;
                            camData.projMatrix = 1.0f;
                            camData.position   = {0.0f,0.0f,0.0f};
                        }
                    }
                }

                lights = scene->GetLights();
            }

            if (benchmark)  benchmark->BeginGpuTimer();
            m_renderQueue.Draw(m_graphicsAPI, camData, lights);
            if (benchmark)  benchmark->EndGpuTimer();
            if (useSceneRT) m_graphicsAPI.BlitSceneToDefault(fbW, fbH);

            // ImGui draw on top of 3D
            m_appManager.RenderGUI();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // end of frame
            glfwSwapBuffers(m_window);

            // Mouse moved this frame
            m_inputManager.SetMousePositionChanged(false);
            m_inputManager.m_mousePosOld = m_inputManager.m_mousePosCurrent;
            m_inputManager.ResetScrollDelta();

            // Late Update before new frame and audio
            m_appManager.LateUpdate(deltaTime);
            m_audioManager.Update(deltaTime);

            if (benchmark) benchmark->EndFrame();
        }
    }

    void Engine::Destroy()
    {
        m_appManager.Destroy();

        // Clean-up ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

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

    void Engine::SetCursorMode(bool _enable)
    {
        auto enable = _enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
        glfwSetInputMode(m_window, GLFW_CURSOR, enable);
    }

    float Engine::GetAspectRatio() const
    {
        // Uses window ptr to asset AP
        int width = 0;
        int height = 0;
        glfwGetWindowSize(m_window, &width, &height);

        if (height > 0)
        {
            return static_cast<float>(width) /
                   static_cast<float>(height);
        }

        return 1.0f;
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

    TextureManager& Engine::GetTextureManager()
    {
        return m_textureManager;
    }

    AudioManager& Engine::GetAudioManager()
    {
        return m_audioManager;
    }

    ApplicationData& Engine::GetAppData()
    {
        return m_appData;
    }
}
