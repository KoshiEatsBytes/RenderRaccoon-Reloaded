
#include <RR.h>
#include "Game.h"
#include "TestSubSystem.h"

int main()
{
    RR::PrintLogo();

    RR::Engine& engine = RR::Engine::GetInstance();
    engine.GetAppManager().RequestSceneLoad<Game>();
    engine.GetAppManager().AddSubSystem<TestSubSystem>();


    if (engine.Init(1920, 1080, "RenderRaccoon"))
    {

        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

