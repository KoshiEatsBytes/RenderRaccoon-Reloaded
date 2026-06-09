
#include <RR.h>

#include "Testing/AudioDemo.h"
#include "Testing/DemoScene.h"
#include "Testing/Game.h"
#include "Testing/TestSubSystem.h"

int main()
{
    RR::PrintLogo();

    RR::Engine& engine = RR::Engine::GetInstance();
    engine.GetAppManager().RequestSceneLoad<Game>();
    engine.GetAppManager().AddSubSystem<TestSubSystem>();


    if (engine.Init(1920, 1080, "RenderRaccoon", 5, 0))
    {

        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

