
#include "MainMenuScene.h"

#include "Benchmark/BenchmarkData.h"
#include "GLFW/glfw3.h"

MainMenuScene::MainMenuScene() : Scene("Main Menu Scene") {}

bool MainMenuScene::Init()
{
    auto& fileSys = RR::Engine::GetInstance().GetFileSystem();
    auto files = fileSys.ListOutputFiles("Benchmarks", {".csv"});

    RR::BenchmarkRun run = RR::BenchmarkParser::ParseBenchmarkCsv(fileSys.LoadOutputFileText(files.front().string()));

    // Test
    const auto sample = run.stats;
    RR::Log("[BENCHMARK] ", static_cast<int>(sample.frameCount), " frames  avg=", sample.avgFrameTimeMs,
        "ms (", sample.avgFps, " fps)  1%low=", sample.low1Pc, "  0.1%low=", sample.low01Pc,
        "  max=", sample.maxFrameTimeMs, "  stutters=", sample.stutterCount,
        "  cpu=", sample.avgCpuMs, "  gpu=", sample.avgGpuMs);

    return true;
}

void MainMenuScene::PreUpdate(float _deltaTime)
{
    auto& input = RR::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        RR::Engine::GetInstance().SetShouldClose(true);
    }
}

void MainMenuScene::Update(float _deltaTime)
{
}

void MainMenuScene::LateUpdate(float _deltaTime)
{
}

void MainMenuScene::Destroy()
{
}
