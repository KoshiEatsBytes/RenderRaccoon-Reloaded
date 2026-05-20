
#include "Game.h"

int main()
{
    RR::printLogo();

    Game* game = new Game{};

    RR::Engine& engine = RR::Engine::GetInstance();
    engine.SetApp(game);

    if (engine.Init(1920, 1080, "First App"))
    {
        RR::print("Application initialization successful! Launching...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

