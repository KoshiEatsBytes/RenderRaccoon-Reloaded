
#include <RR.h>

#include "Scenes/ArtefactMenu.h"
#include "Benchmark/BenchmarkSubSystem.h"
#include "Testing/AudioDemo.h"
#include "Testing/DemoScene.h"
#include "Testing/Game.h"
#include "Testing/TestSubSystem.h"

int main()
{
    RR::PrintLogo();

    RR::Engine& engine = RR::Engine::GetInstance();
    engine.GetAppManager().RequestSceneLoad<ArtefactMenu>();
    engine.GetAppManager().AddSubSystem<RR::BenchmarkSubSystem>();


    if (engine.Init(1920, 1080, "RenderRaccoon", 5, 0))
    {

        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

