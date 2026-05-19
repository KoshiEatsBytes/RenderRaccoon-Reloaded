
#include "Engine.h"
#include "Application.h"

namespace rr
{
    Engine::Engine()
    = default;

    Engine::~Engine()
    = default;

    bool Engine::Init() const
    {
        if (!_application)
        {
            // No valid application
            warn("Tried initializing invalid application");
            return false;
        }

        return _application->Init();
    }

    void Engine::Launch()
    {
        if (!_application)
        {
            warn("Tried launching invalid application");
            return;
        }

        _lastTimePoint = std::chrono::steady_clock::now();

        // Main application loop
        while (!_application->ShouldClose())
        {
            // compute delta time
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - _lastTimePoint).count();
            _lastTimePoint = now;

            _application->Update(deltaTime);
        }
    }

    void Engine::Destroy()
    {
        if (_application)
        {
            _application->Destroy();
            _application.reset();
            return;
        }

        warn("Tried destroying invalid application");
    }

    void Engine::SetApp(Application* app)
    {
        _application.reset(app);
    }

    Application* Engine::GetApp() const
    {
        return _application.get();
    }
}
