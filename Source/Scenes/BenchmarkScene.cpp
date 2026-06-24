
#include "BenchmarkScene.h"

#include "MainMenuScene.h"
#include "Benchmark/BenchmarkRunPresets.hpp"
#include "Components/FreeCameraComponent.h"

// PUBLIC --------------------------------------------------------------------------------------------------------------

BenchmarkScene::BenchmarkScene(const RR::RunInfo& _runInfo, const WORLDGEN::WorldGenConfig& _config)
    : VoxelScene(_runInfo, _config)
{
}

BenchmarkScene::BenchmarkScene(const RR::RunInfo& _runInfo)
    : VoxelScene(_runInfo)
{
}

BenchmarkScene::~BenchmarkScene()
= default;

// PROTECTED -----------------------------------------------------------------------------------------------------------

void BenchmarkScene::OnInit()
{
    // Benchmarks are non-resumable from pause
    SetResumable(false);
    SetPrimaryButtonText("RESTART BENCHMARK");
    SetSecondaryButtonText("EXIT TO MAIN MENU");

    // Create free cam
    m_cam     = CreateObject("BenchmarkCam");
    m_camComp = m_cam->AddComponent<RR::FreeCameraComponent>();
    m_cam->SetPosition(vec3(0.f, 120.f, 0.f));
    SetMainCamera(m_cam);

    SetCursorEnabled(false);
    m_camComp->SetDiscardInput(true);

    // PLACEHOLDER
    m_bench = RR::Engine::GetInstance().GetAppManager().GetSubSystem<RR::BenchmarkSubSystem>();
}

void BenchmarkScene::OnUpdate(float _deltaTime)
{
    if (m_paused) return;

    // PLACEHOLDER rotate camera around
    m_yaw += 0.75f * _deltaTime;
    const quat qYaw = glm::angleAxis(m_yaw,glm::vec3(0.0f, 1.0f, 0.0f));
    m_cam->SetWorldRotation(qYaw);

    if (m_discard < 10)
    {
        m_discard++;

        if (m_discard == 8)
        {
            m_bench->RequestStartLogging(m_runInfo, 2);
        }

        return;
    }

    // PLACEHOLDER test scene sequence
    if (!m_fired)
    {
        m_timer += _deltaTime;

        if (m_timer >= 4.f)
        {
            m_fired = true;
            m_bench->RequestStopLogging();
            LoadNextScene();
        }
    }
}

void BenchmarkScene::OnPauseEnter()
{
    VoxelScene::OnPauseEnter();
}

void BenchmarkScene::OnPausePrimary()
{
    auto& appMan = RR::Engine::GetInstance().GetAppManager();

    // if deterministic and restart prompted, reload entire sequence
    if (m_runInfo.deterministic)
    {
        using namespace DETERMINISTIC;

        gCurrentSceneStep = static_cast<uInt8>(SCENE::BASELINE);
        appMan.RequestSceneLoad<BenchmarkScene>(GetRunPreset(SCENE::BASELINE));
        return;
    }

    // For custom only, restart this exact scene
    appMan.RequestSceneLoad<BenchmarkScene>(m_runInfo, m_genConfig);
}

void BenchmarkScene::OnPauseSecondary()
{
    // User has quit
    auto& appMan = RR::Engine::GetInstance().GetAppManager();
    appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::FAILED);
}

void BenchmarkScene::LoadNextScene()
{
    auto& appMan = RR::Engine::GetInstance().GetAppManager();

    if (!m_runInfo.deterministic)
    {
        appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::CUSTOM);
        return;
    }

    using namespace DETERMINISTIC;

    // Hit end of sequence, return to main menu
    if (gCurrentSceneStep + 1 == kSceneCount)
    {
        RR::Success("[DETERMINISTIC BENCHMARK] Deterministic benchmark has concluded successfully");
        appMan.RequestSceneLoad<MainMenuScene>(BENCH_SUCCESS::DETERMINISTIC);
        return;
    }

    // Load next scene in sequence
    gCurrentSceneStep++;
    RR::RunInfo info = GetRunPreset(static_cast<SCENE>(gCurrentSceneStep));
    appMan.RequestSceneLoad<BenchmarkScene>(info);
}










