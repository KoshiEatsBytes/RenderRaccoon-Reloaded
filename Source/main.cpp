
#include <RR.h>

int main()
{
    RR::PrintLogo();

    RR::Engine& engine = RR::Engine::GetInstance();

    if (engine.Init(1920, 1080, "RenderRaccoon"))
    {
        RR::Log("Launching application...");

        engine.Launch();
    }

    engine.Destroy();

    return 0;
}

