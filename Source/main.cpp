
#include "Game.h"

int main()
{
    RR::PrintLogo();

    Game* game = new Game{};

    RR::Engine& engine = RR::Engine::GetInstance();
    engine.SetApp(game);

    if (engine.Init(1920, 1080, "RenderRaccoon"))
    {
        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

