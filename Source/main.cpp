
#include <RR.h>

#include "Scenes/MainMenuScene.h"
#include "Benchmark/BenchmarkSubSystem.h"

int main()
{
    RR::PrintLogo();

    RR::Engine& engine = RR::Engine::GetInstance();

    // Load benchmark and main scene
    engine.GetAppManager().AddSubSystem<RR::BenchmarkSubSystem>();
    engine.GetAppManager().RequestSceneLoad<MainMenuScene>();

    if (engine.Init(1920, 1080, "RenderRaccoon", 5, 0))
    {
        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

