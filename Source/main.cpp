
#include <RR.h>

#include "Scenes/ArtefactMenu.h"
#include "Benchmark/BenchmarkSubSystem.h"
#include "Scenes/FreeRoam.h"
#include "Testing/AudioDemo.h"
#include "Testing/DemoScene.h"
#include "Testing/Game.h"
#include "Testing/TestSubSystem.h"

int main()
{
    RR::PrintLogo();

    RR::Engine& engine = RR::Engine::GetInstance();
    engine.GetAppManager().AddSubSystem<RR::BenchmarkSubSystem>();

    WORLDGEN::WorldGenConfig cfg;
    RR::RunInfo runInfo;
    runInfo.seed = 2498846564;
    engine.GetAppManager().RequestSceneLoad<FreeRoam>(runInfo, cfg);


    if (engine.Init(1920, 1080, "RenderRaccoon", 5, 0))
    {

        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

