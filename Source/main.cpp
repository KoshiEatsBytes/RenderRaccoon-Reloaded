
#include "Game.h"

int main()
{
    rr::printLogo();

    Game* game = new Game{};

    rr::Engine engine;
    engine.SetApp(game);

    if (engine.Init(1920, 1080, "First App"))
    {
        rr::print("Application initialization successful! Launching...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

